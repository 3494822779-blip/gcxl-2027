#include <rclcpp/rclcpp.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <block_interfaces/msg/frame_metadata.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gstnvdsmeta.h>
#include <nvdsmeta.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <future>
#include <map>
#include <mutex>
#include <thread>
#include "v4l2_source.hpp"
#include "target_cache.hpp"

class NvmmPerception : public rclcpp::Node {
  using Metadata = block_interfaces::msg::FrameMetadata;
  GstElement *pipeline_ = nullptr, *source_ = nullptr;
  GstBus *bus_ = nullptr;
  std::unique_ptr<V4l2Source> camera_;
  std::thread reader_;
  std::atomic<bool> stopping_{false}, failed_{false};
  int width_, height_, fps_, wait_ms_;
  int64_t capture_origin_ = 0;
  const pid_t parent_pid_ = getppid();
  std::string frame_id_;
  std::atomic<double> confidence_{0.75};
  TargetCache targets_;
  std::mutex headers_mutex_;
  std::map<GstClockTime, std_msgs::msg::Header> headers_;
  rclcpp::Publisher<Metadata>::SharedPtr detections_, tracks_;
  rclcpp::Subscription<block_interfaces::msg::Target>::SharedPtr target_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
  OnSetParametersCallbackHandle::SharedPtr params_;
  std::atomic<uint64_t> captured_{0}, detected_{0}, tracked_{0}, shown_{0}, osd_matched_{0}, osd_missed_{0};
  std::atomic<uint64_t> missing_headers_{0}, sequence_gaps_{0}, camera_errors_{0}, latency_ns_{0};
  std::chrono::steady_clock::time_point report_time_ = std::chrono::steady_clock::now();
  uint64_t previous_capture_ = 0, previous_detect_ = 0, previous_track_ = 0, previous_show_ = 0;

  void fail(const std::string &message) {
    if (!failed_.exchange(true)) RCLCPP_ERROR(get_logger(), "%s", message.c_str());
    stopping_ = true;
    targets_.stop();
  }
  std::optional<std_msgs::msg::Header> header(GstClockTime pts) {
    std::lock_guard<std::mutex> lock(headers_mutex_);
    auto found = headers_.find(pts);
    if (found == headers_.end()) {
      auto misses = ++missing_headers_;
      if (misses <= 5) RCLCPP_WARN(get_logger(), "Missing header pts=%lu pending=[%lu,%lu] count=%zu", pts,
        headers_.empty() ? 0 : headers_.begin()->first, headers_.empty() ? 0 : headers_.rbegin()->first, headers_.size());
      return std::nullopt;
    }
    return found->second;
  }
  // Verify negotiated memory at runtime, not merely the pipeline description.
  static GstPadProbeReturn check_memory(GstPad *pad, GstPadProbeInfo *info, gpointer user) {
    if (!(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM)) return GST_PAD_PROBE_OK;
    auto *event = GST_PAD_PROBE_INFO_EVENT(info);
    if (GST_EVENT_TYPE(event) != GST_EVENT_CAPS) return GST_PAD_PROBE_OK;
    auto *self = static_cast<NvmmPerception *>(user);
    GstCaps *caps = nullptr;
    gst_event_parse_caps(event, &caps);
    bool nvmm = caps && gst_caps_get_size(caps) &&
      gst_caps_features_contains(gst_caps_get_features(caps, 0), "memory:NVMM");
    gchar *text = caps ? gst_caps_to_string(caps) : g_strdup("missing caps");
    auto *parent = gst_pad_get_parent_element(pad);
    const char *name = parent ? GST_ELEMENT_NAME(parent) : "unknown";
    if (!nvmm) self->fail(std::string("Non-NVMM raw image at ") + name + ": " + text);
    else RCLCPP_INFO(self->get_logger(), "NVMM verified at %s: %s", name, text);
    if (parent) gst_object_unref(parent);
    g_free(text);
    return nvmm ? GST_PAD_PROBE_OK : GST_PAD_PROBE_DROP;
  }
  Metadata extract(NvDsFrameMeta *frame, const std_msgs::msg::Header &h, bool tracking) {
    Metadata out;
    out.header = h; out.width = width_; out.height = height_;
    for (auto *item = frame->obj_meta_list; item; item = item->next) {
      auto *obj = static_cast<NvDsObjectMeta *>(item->data);
      if (obj->class_id < 0 || obj->class_id >= 4 ||
          (tracking && obj->object_id == UNTRACKED_OBJECT_ID)) continue;
      const auto &rect = obj->rect_params;
      if (!std::isfinite(rect.left) || !std::isfinite(rect.top) ||
          !std::isfinite(rect.width) || !std::isfinite(rect.height)) continue;
      float x = std::max(0.f, rect.left), y = std::max(0.f, rect.top);
      float right = std::min(float(width_), rect.left + rect.width);
      float bottom = std::min(float(height_), rect.top + rect.height);
      if (right - x < 2.f || bottom - y < 2.f) continue;
      block_interfaces::msg::Box b;
      b.track_id = obj->object_id; b.class_id = obj->class_id; b.confidence = obj->confidence;
      b.x = x; b.y = y; b.width = right - x; b.height = bottom - y;
      b.observed = obj->confidence >= 0;
      out.boxes.push_back(b);
    }
    return out;
  }
  static GstPadProbeReturn detected(GstPad *, GstPadProbeInfo *info, gpointer user) {
    auto *self = static_cast<NvmmPerception *>(user);
    try {
      auto *batch = gst_buffer_get_nvds_batch_meta(GST_PAD_PROBE_INFO_BUFFER(info));
      if (!batch) return GST_PAD_PROBE_OK;
      for (auto *f = batch->frame_meta_list; f; f = f->next) {
        auto *frame = static_cast<NvDsFrameMeta *>(f->data);
        // Preserve the old detector->tracker confidence gate before tracker consumes metadata.
        for (auto *o = frame->obj_meta_list; o;) {
          auto *next = o->next;
          auto *obj = static_cast<NvDsObjectMeta *>(o->data);
          auto &r = obj->rect_params;
          bool valid = obj->class_id >= 0 && obj->class_id < 4 && std::isfinite(obj->confidence) &&
                       obj->confidence >= self->confidence_.load() && obj->confidence <= 1 &&
                       std::isfinite(r.left) && std::isfinite(r.top) &&
                       std::isfinite(r.width) && std::isfinite(r.height);
          if (valid) {
            float right = std::min(float(self->width_), r.left + r.width);
            float bottom = std::min(float(self->height_), r.top + r.height);
            r.left = std::max(0.f, r.left); r.top = std::max(0.f, r.top);
            r.width = right - r.left; r.height = bottom - r.top;
            valid = r.width >= 2 && r.height >= 2;
          }
          if (!valid) nvds_remove_obj_meta_from_frame(frame, obj);
          else {
            obj->detector_bbox_info.org_bbox_coords.left = r.left;
            obj->detector_bbox_info.org_bbox_coords.top = r.top;
            obj->detector_bbox_info.org_bbox_coords.width = r.width;
            obj->detector_bbox_info.org_bbox_coords.height = r.height;
          }
          o = next;
        }
        auto h = self->header(frame->buf_pts);
        if (h) { self->detections_->publish(self->extract(frame, *h, false)); ++self->detected_; }
      }
    } catch (const std::exception &e) { self->fail(e.what()); return GST_PAD_PROBE_DROP; }
    return GST_PAD_PROBE_OK;
  }
  static GstPadProbeReturn tracked(GstPad *, GstPadProbeInfo *info, gpointer user) {
    auto *self = static_cast<NvmmPerception *>(user);
    try {
      auto *batch = gst_buffer_get_nvds_batch_meta(GST_PAD_PROBE_INFO_BUFFER(info));
      if (!batch) return GST_PAD_PROBE_OK;
      for (auto *f = batch->frame_meta_list; f; f = f->next) {
        auto *frame = static_cast<NvDsFrameMeta *>(f->data);
        auto h = self->header(frame->buf_pts);
        if (!h) continue;
        self->tracks_->publish(self->extract(frame, *h, true)); ++self->tracked_;
        auto elapsed = self->now().nanoseconds() - TargetCache::key(*h).first;
        if (elapsed > 0) self->latency_ns_ += uint64_t(elapsed);
      }
    } catch (const std::exception &e) { self->fail(e.what()); return GST_PAD_PROBE_DROP; }
    return GST_PAD_PROBE_OK;
  }
  static GstPadProbeReturn overlay(GstPad *, GstPadProbeInfo *info, gpointer user) {
    auto *self = static_cast<NvmmPerception *>(user);
    if (self->stopping_) return GST_PAD_PROBE_DROP;
    try {
      auto *batch = gst_buffer_get_nvds_batch_meta(GST_PAD_PROBE_INFO_BUFFER(info));
      if (!batch) return GST_PAD_PROBE_OK;
      for (auto *f = batch->frame_meta_list; f; f = f->next) {
        auto *frame = static_cast<NvDsFrameMeta *>(f->data);
        auto h = self->header(frame->buf_pts);
        if (!h) continue;
        auto target = self->targets_.wait(*h, std::chrono::milliseconds(self->wait_ms_));
        ++self->shown_;
        if (!target) { ++self->osd_missed_; continue; }
        ++self->osd_matched_;
        if (!target->valid || target->image_width != unsigned(self->width_) ||
            target->image_height != unsigned(self->height_) ||
            !std::isfinite(target->x) || !std::isfinite(target->y)) continue;
        auto *display = nvds_acquire_display_meta_from_pool(batch);
        auto x = unsigned(std::clamp(target->x, 0., double(self->width_ - 1)));
        auto y = unsigned(std::clamp(target->y, 0., double(self->height_ - 1)));
        auto &circle = display->circle_params[display->num_circles++];
        circle.xc = x; circle.yc = y; circle.radius = 5;
        circle.circle_color = {0., 1., 1., 1.}; circle.has_bg_color = 1; circle.bg_color = circle.circle_color;
        if (target->observed && std::isfinite(target->raw_x) && std::isfinite(target->raw_y)) {
          auto &raw = display->circle_params[display->num_circles++];
          raw.xc = unsigned(std::clamp(target->raw_x, 0., double(self->width_ - 1)));
          raw.yc = unsigned(std::clamp(target->raw_y, 0., double(self->height_ - 1)));
          raw.radius = 5; raw.circle_color = {1., 1., 0., 1.}; raw.has_bg_color = 1; raw.bg_color = raw.circle_color;
        }
        auto &line = display->line_params[display->num_lines++];
        line.x1 = self->width_ / 2; line.y1 = self->height_ / 2; line.x2 = x; line.y2 = y;
        line.line_width = 2; line.line_color = {0., 1., 1., 1.};
        auto &text = display->text_params[display->num_labels++];
        text.display_text = g_strdup_printf("target=%lu observed=%s", target->track_id, target->observed ? "yes" : "no");
        text.x_offset = 8; text.y_offset = 12; text.font_params.font_name = const_cast<char *>("Sans");
        text.font_params.font_size = 16; text.font_params.font_color = {0., 1., 1., 1.};
        nvds_add_display_meta_to_frame(frame, display);
      }
    } catch (const std::exception &e) { self->fail(e.what()); return GST_PAD_PROBE_DROP; }
    return GST_PAD_PROBE_OK;
  }
  void capture() {
    try {
      const auto origin = capture_origin_;
      GstClockTime last_pts = 0;
      int idle = 0;
      while (!stopping_ && rclcpp::ok()) {
        V4l2Source::Frame frame;
        if (!camera_->read(frame)) {
          if (++idle >= 6) throw std::runtime_error("Camera stalled: no valid frame in six reads");
          continue;
        }
        idle = 0;
        if (stopping_) break;
        GstClockTime pts = std::max<GstClockTime>(last_pts + 1, std::max<int64_t>(1, frame.timestamp_ns - origin));
        last_pts = pts;
        std_msgs::msg::Header h;
        auto age = std::max<int64_t>(0, monotonic_ns() - frame.timestamp_ns);
        h.stamp = rclcpp::Time(std::max<int64_t>(0, now().nanoseconds() - age));
        h.frame_id = frame_id_;
        {
          std::lock_guard<std::mutex> lock(headers_mutex_);
          headers_[pts] = h;
          while (headers_.size() > 120) headers_.erase(headers_.begin());
        }
        auto *buffer = gst_buffer_new_allocate(nullptr, frame.jpeg.size(), nullptr);
        if (!buffer) throw std::runtime_error("Cannot allocate compressed JPEG buffer");
        gst_buffer_fill(buffer, 0, frame.jpeg.data(), frame.jpeg.size());
        GST_BUFFER_PTS(buffer) = pts; GST_BUFFER_DTS(buffer) = pts;
        GST_BUFFER_DURATION(buffer) = GST_SECOND / fps_;
        GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_MARKER);
        auto result = gst_app_src_push_buffer(GST_APP_SRC(source_), buffer);
        if (result != GST_FLOW_OK && !stopping_) throw std::runtime_error("GStreamer rejected JPEG frame");
        ++captured_;
        sequence_gaps_ = camera_->sequence_gaps; camera_errors_ = camera_->error_frames;
      }
    } catch (const std::exception &e) { if (!stopping_) fail(e.what()); }
    camera_.reset();
  }
  void monitor() {
    if (getppid() != parent_pid_) fail("Launching process disappeared");
    while (auto *msg = gst_bus_pop_filtered(bus_, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) {
      if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *error = nullptr; gchar *debug = nullptr;
        gst_message_parse_error(msg, &error, &debug);
        fail(std::string(error->message) + " (" + (debug ? debug : "") + ")");
        g_error_free(error); g_free(debug);
      } else fail("Video stream/display ended");
      gst_message_unref(msg);
    }
    if (failed_) { rclcpp::shutdown(); return; }
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - report_time_).count();
    if (dt < 5) return;
    uint64_t c = captured_, d = detected_, t = tracked_, s = shown_;
    RCLCPP_INFO(get_logger(), "fps capture=%.1f detect=%.1f track=%.1f display=%.1f; "
                "track_latency_mean_ms=%.2f sequence_gaps=%lu error_frames=%lu osd_match=%lu osd_timeout=%lu",
                (c-previous_capture_)/dt, (d-previous_detect_)/dt, (t-previous_track_)/dt,
                (s-previous_show_)/dt, t ? double(latency_ns_)/t/1e6 : 0.,
                sequence_gaps_.load(), camera_errors_.load(), osd_matched_.load(), osd_missed_.load());
    previous_capture_=c; previous_detect_=d; previous_track_=t; previous_show_=s; report_time_=now;
  }
  void add_probe(const char *name, GstPadProbeCallback callback, bool verify = true) {
    auto *element = gst_bin_get_by_name(GST_BIN(pipeline_), name);
    if (!element) throw std::runtime_error(std::string("Missing element ") + name);
    auto *pad = gst_element_get_static_pad(element, "src");
    if (callback) gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, callback, this, nullptr);
    if (verify) gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, check_memory, this, nullptr);
    gst_object_unref(pad); gst_object_unref(element);
  }
 public:
  NvmmPerception() : Node("perception") {
    try {
      rcl_interfaces::msg::ParameterDescriptor fixed; fixed.read_only = true;
      auto device = declare_parameter("source", std::string("/dev/video0"), fixed);
      width_ = declare_parameter("width", 1280, fixed); height_ = declare_parameter("height", 720, fixed);
      fps_ = declare_parameter("fps", 60, fixed);
      frame_id_ = declare_parameter("frame_id", std::string("camera_optical_frame"), fixed);
      wait_ms_ = declare_parameter("osd_wait_ms", 50, fixed);
      bool display = declare_parameter("display", true, fixed);
      bool headless = declare_parameter("headless", false, fixed);
      auto infer = declare_parameter("inference_config", std::string(""), fixed);
      auto tracker = declare_parameter("tracker_config", std::string(""), fixed);
      auto library = declare_parameter("tracker_library", std::string("/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"), fixed);
      int tw = declare_parameter("tracker_width", 960, fixed), th = declare_parameter("tracker_height", 544, fixed);
      confidence_ = declare_parameter("confidence_threshold", 0.75);
      if (get_parameter("use_sim_time").as_bool() || width_ <= 0 || height_ <= 0 || width_%4 || height_%2 ||
          fps_ <= 0 || fps_ > 240 || wait_ms_ < 0 || wait_ms_ > 50 || tw <= 0 || th <= 0 ||
          tw%32 || th%32 || infer.empty() || tracker.empty() || !std::isfinite(confidence_) ||
          confidence_ < .01 || confidence_ > 1.) throw std::runtime_error("Invalid NVMM camera/pipeline parameters (physical camera requires real time)");
      GKeyFile *key = g_key_file_new(); GError *error = nullptr;
      if (!g_key_file_load_from_file(key, infer.c_str(), G_KEY_FILE_NONE, &error)) {
        std::string message = error ? error->message : "Cannot load inference config";
        if (error) g_error_free(error);
        g_key_file_unref(key); throw std::runtime_error(message);
      }
      int classes = g_key_file_get_integer(key, "property", "num-detected-classes", nullptr);
      double floor = g_key_file_get_double(key, "class-attrs-all", "pre-cluster-threshold", nullptr);
      g_key_file_unref(key);
      if (classes != 4 || floor > .01) throw std::runtime_error("Inference config requires four classes and candidate floor <=0.01");
      params_ = add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter> &ps) {
        rcl_interfaces::msg::SetParametersResult result; result.successful = true;
        double value = confidence_;
        for (const auto &p : ps) if (p.get_name() == "confidence_threshold") {
          if (p.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE || !std::isfinite(p.as_double()) ||
              p.as_double() < .01 || p.as_double() > 1.) {
            result.successful = false; result.reason = "confidence_threshold must be in [0.01,1]"; return result;
          }
          value = p.as_double();
        }
        confidence_ = value; return result;
      });
      detections_ = create_publisher<Metadata>("detections_meta", rclcpp::SensorDataQoS());
      tracks_ = create_publisher<Metadata>("tracks_meta", rclcpp::SensorDataQoS());
      if (display) target_sub_ = create_subscription<block_interfaces::msg::Target>("target/filtered", rclcpp::SensorDataQoS(),
        [this](block_interfaces::msg::Target::ConstSharedPtr t) { targets_.put(*t); });
      gst_init(nullptr, nullptr);
      // V4L2 already supplies one complete JPEG per buffer. jpegparse would retime
      // frames to a nominal cadence, losing the exact acquisition PTS association.
      std::string desc = "appsrc name=input is-live=true format=time block=false max-bytes=0 max-buffers=2 leaky-type=downstream ! "
        "nvjpegdec name=decode ! video/x-raw(memory:NVMM),format=Y42B ! "
        "nvvidconv name=convert ! video/x-raw(memory:NVMM),format=NV12 ! mux.sink_0 "
        "nvstreammux name=mux batch-size=1 live-source=true width=" + std::to_string(width_) +
        " height=" + std::to_string(height_) + " batched-push-timeout=16000 buffer-pool-size=12 ! "
        "nvinfer name=infer ! nvtracker name=tracker ! ";
      if (display) desc += "queue name=display_queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream ! "
        "nvvidconv name=rgba ! video/x-raw(memory:NVMM),format=RGBA ! identity name=overlay ! "
        "nvdsosd name=osd process-mode=1 display-bbox=true display-text=true ! " +
        std::string(headless ? "fakesink sync=false async=false" : "nvegltransform ! nveglglessink sync=false async=false");
      else desc += "fakesink sync=false async=false";
      pipeline_ = gst_parse_launch(desc.c_str(), &error);
      if (error) { std::string message = error->message; g_error_free(error); throw std::runtime_error(message); }
      source_ = gst_bin_get_by_name(GST_BIN(pipeline_), "input");
      auto *caps = gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_,
                                       "framerate", GST_TYPE_FRACTION, fps_, 1, nullptr);
      gst_app_src_set_caps(GST_APP_SRC(source_), caps); gst_caps_unref(caps);
      auto *inference = gst_bin_get_by_name(GST_BIN(pipeline_), "infer");
      g_object_set(inference, "config-file-path", infer.c_str(), nullptr); gst_object_unref(inference);
      auto *tracking = gst_bin_get_by_name(GST_BIN(pipeline_), "tracker");
      g_object_set(tracking, "ll-lib-file", library.c_str(), "ll-config-file", tracker.c_str(),
                   "tracker-width", tw, "tracker-height", th, nullptr); gst_object_unref(tracking);
      add_probe("decode", nullptr); add_probe("convert", nullptr); add_probe("mux", nullptr);
      add_probe("infer", detected); add_probe("tracker", tracked);
      if (display) { add_probe("rgba", nullptr); add_probe("overlay", overlay); add_probe("osd", nullptr); }
      bus_ = gst_element_get_bus(pipeline_);
      if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
        throw std::runtime_error("NVMM pipeline failed to start");
      capture_origin_ = monotonic_ns();
      camera_ = std::make_unique<V4l2Source>(device, width_, height_, fps_, [this](const std::string &text) {
        RCLCPP_INFO(get_logger(), "V4L2 default: %s", text.c_str());
      });
      reader_ = std::thread([this] { capture(); });
      timer_ = create_wall_timer(std::chrono::milliseconds(100), [this] { monitor(); });
      RCLCPP_INFO(get_logger(), "NVMM perception started; ROS publishes metadata only");
    } catch (...) { stop(); throw; }
  }
  bool failed() const { return failed_; }
  void stop() {
    stopping_ = true; targets_.stop();
    if (reader_.joinable()) reader_.join();
    camera_.reset();
    if (pipeline_) gst_element_set_state(pipeline_, GST_STATE_NULL);
    if (bus_) { gst_object_unref(bus_); bus_ = nullptr; }
    if (source_) { gst_object_unref(source_); source_ = nullptr; }
    if (pipeline_) { gst_object_unref(pipeline_); pipeline_ = nullptr; }
  }
  ~NvmmPerception() override { stop(); }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  std::shared_ptr<NvmmPerception> node;
  int result = 0;
  try { node = std::make_shared<NvmmPerception>(); rclcpp::spin(node); if (node->failed()) result = 1; }
  catch (const std::exception &e) { RCLCPP_FATAL(rclcpp::get_logger("perception"), "%s", e.what()); result = 1; }
  if (rclcpp::ok()) rclcpp::shutdown();
  auto done = std::make_shared<std::promise<void>>(); auto finished = done->get_future();
  std::thread([node = std::move(node), done]() mutable { node.reset(); done->set_value(); }).detach();
  if (finished.wait_for(std::chrono::seconds(3)) != std::future_status::ready) {
    std::fprintf(stderr, "NVMM teardown exceeded 3 seconds; exiting\n"); std::fflush(stderr); std::_Exit(1);
  }
  return result;
}
