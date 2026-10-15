#include <rclcpp/rclcpp.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <block_interfaces/msg/frame.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gstnvdsmeta.h>
#include <nvdsmeta.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <stdexcept>

#ifdef TRACKER_NODE
constexpr bool tracking = true;
#else
constexpr bool tracking = false;
#endif

class Bridge : public rclcpp::Node {
  using Frame = block_interfaces::msg::Frame;
  GstElement *pipeline_ = nullptr, *source_ = nullptr;
  GstBus *bus_ = nullptr;
  std::mutex mutex_;
  std::map<GstClockTime, Frame> pending_;
  uint64_t sequence_ = 0;
  int width_, height_;
  std::atomic<double> confidence_{0.25};
  rclcpp::Publisher<Frame>::SharedPtr pub_;
  rclcpp::Subscription<Frame>::SharedPtr frames_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr images_;
  rclcpp::TimerBase::SharedPtr timer_;
  OnSetParametersCallbackHandle::SharedPtr params_;

  static GstPadProbeReturn inject(GstPad *, GstPadProbeInfo *info, gpointer user) {
    auto *self = static_cast<Bridge *>(user);
    auto *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    auto *batch = gst_buffer_get_nvds_batch_meta(buffer);
    if (!batch) return GST_PAD_PROBE_OK;
    std::lock_guard<std::mutex> guard(self->mutex_);
    for (auto *f = batch->frame_meta_list; f; f = f->next) {
      auto *meta = static_cast<NvDsFrameMeta *>(f->data);
      auto found = self->pending_.find(meta->buf_pts);
      if (found == self->pending_.end()) continue;
      meta->bInferDone = TRUE;
      for (const auto &box : found->second.boxes) {
        if (!box.observed || box.class_id < 0 || box.class_id >= 4 ||
            !std::isfinite(box.confidence) || box.confidence < 0 || box.confidence > 1 ||
            !std::isfinite(box.x) || !std::isfinite(box.y) ||
            !std::isfinite(box.width) || !std::isfinite(box.height) ||
            box.width <= 0 || box.height <= 0 || box.x < 0 || box.y < 0 ||
            box.x + box.width > self->width_ + 1 || box.y + box.height > self->height_ + 1) continue;
        auto *obj = nvds_acquire_obj_meta_from_pool(batch);
        obj->unique_component_id = 1;
        obj->class_id = box.class_id;
        obj->confidence = box.confidence;
        obj->object_id = UNTRACKED_OBJECT_ID;
        obj->rect_params.left = box.x;
        obj->rect_params.top = box.y;
        obj->rect_params.width = box.width;
        obj->rect_params.height = box.height;
        obj->detector_bbox_info.org_bbox_coords.left = box.x;
        obj->detector_bbox_info.org_bbox_coords.top = box.y;
        obj->detector_bbox_info.org_bbox_coords.width = box.width;
        obj->detector_bbox_info.org_bbox_coords.height = box.height;
        nvds_add_obj_meta_to_frame(meta, obj, nullptr);
      }
    }
    return GST_PAD_PROBE_OK;
  }

  static GstPadProbeReturn extract(GstPad *, GstPadProbeInfo *info, gpointer user) {
    auto *self = static_cast<Bridge *>(user);
    auto *batch = gst_buffer_get_nvds_batch_meta(GST_PAD_PROBE_INFO_BUFFER(info));
    if (!batch) return GST_PAD_PROBE_OK;
    for (auto *f = batch->frame_meta_list; f; f = f->next) {
      auto *meta = static_cast<NvDsFrameMeta *>(f->data);
      Frame out;
      {
        std::lock_guard<std::mutex> guard(self->mutex_);
        auto found = self->pending_.find(meta->buf_pts);
        if (found == self->pending_.end()) continue;
        out = std::move(found->second);
        self->pending_.erase(found);
      }
      out.boxes.clear();
      for (auto *o = meta->obj_meta_list; o; o = o->next) {
        auto *obj = static_cast<NvDsObjectMeta *>(o->data);
        if (obj->class_id < 0 || obj->class_id >= 4) continue;
        if (!tracking && obj->confidence < self->confidence_.load()) continue;
        if (tracking && obj->object_id == UNTRACKED_OBJECT_ID) continue;
        block_interfaces::msg::Box b;
        b.track_id = obj->object_id;
        b.class_id = obj->class_id;
        b.confidence = obj->confidence;
        b.x = obj->rect_params.left;
        b.y = obj->rect_params.top;
        b.width = obj->rect_params.width;
        b.height = obj->rect_params.height;
        // DeepStream uses confidence=-0.1 for tracker-only output.
        b.observed = obj->confidence >= 0;
        out.boxes.push_back(b);
      }
      self->pub_->publish(out);
    }
    return GST_PAD_PROBE_OK;
  }

  void push(Frame frame) {
    const auto &im = frame.image;
    if (im.width != static_cast<unsigned>(width_) || im.height != static_cast<unsigned>(height_) ||
        im.encoding != "rgb8" || im.step < im.width * 3 || im.data.size() < size_t(im.step) * im.height) {
      RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 3000, "Expected configured width/height, rgb8 image with valid row stride");
      return;
    }
    auto *buffer = gst_buffer_new_allocate(nullptr, size_t(width_) * height_ * 3, nullptr);
    if (!buffer) throw std::runtime_error("Cannot allocate video buffer");
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    for (int y = 0; y < height_; ++y)
      std::memcpy(map.data + size_t(y) * width_ * 3, im.data.data() + size_t(y) * im.step, width_ * 3);
    gst_buffer_unmap(buffer, &map);
    // Private monotonic PTS correlates pipeline frames. ROS acquisition stamps are preserved.
    GstClockTime pts = (++sequence_) * GST_SECOND / 30;
    GST_BUFFER_PTS(buffer) = pts;
    GST_BUFFER_DURATION(buffer) = GST_SECOND / 30;
    {
      std::lock_guard<std::mutex> guard(mutex_);
      if (pending_.size() >= 8) { gst_buffer_unref(buffer); return; }
      pending_.emplace(pts, std::move(frame));
    }
    if (gst_app_src_push_buffer(GST_APP_SRC(source_), buffer) != GST_FLOW_OK) {
      std::lock_guard<std::mutex> guard(mutex_);
      pending_.erase(pts);
      RCLCPP_ERROR(get_logger(), "DeepStream appsrc rejected frame");
    }
  }

 public:
  Bridge() : Node(tracking ? "tracker" : "detector") {
    rcl_interfaces::msg::ParameterDescriptor fixed;
    fixed.read_only = true;
    width_ = declare_parameter("width", 640, fixed);
    height_ = declare_parameter("height", 480, fixed);
    if (width_ <= 0 || height_ <= 0 || width_ % 4 != 0) throw std::runtime_error("width must be positive and divisible by 4; height positive");
    auto config = declare_parameter("config_file", std::string(""), fixed);
    if (config.empty()) throw std::runtime_error("config_file must name a DeepStream inference/tracker config");
    std::string lib;
    if (tracking) {
      lib = declare_parameter("tracker_library", std::string("/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"), fixed);
    } else {
      confidence_ = declare_parameter("confidence_threshold", 0.25);
      if (!std::isfinite(confidence_) || confidence_ < 0.01 || confidence_ > 1.) throw std::runtime_error("confidence_threshold must be in [0.01,1]");
      GKeyFile *key = g_key_file_new();
      GError *err = nullptr;
      if (!g_key_file_load_from_file(key, config.c_str(), G_KEY_FILE_NONE, &err)) {
        std::string message = err ? err->message : "cannot read config";
        if (err) g_error_free(err);
        g_key_file_unref(key);
        throw std::runtime_error(message);
      }
      int classes = g_key_file_get_integer(key, "property", "num-detected-classes", nullptr);
      double floor = g_key_file_get_double(key, "class-attrs-all", "pre-cluster-threshold", nullptr);
      g_key_file_unref(key);
      if (classes != 4 || floor > 0.01) throw std::runtime_error("Inference config requires 4 classes and pre-cluster-threshold <= 0.01");
      params_ = add_on_set_parameters_callback([this](const std::vector<rclcpp::Parameter> &ps) {
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = true;
        double next = confidence_;
        for (const auto &p : ps) if (p.get_name() == "confidence_threshold") {
          if (p.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE || !std::isfinite(p.as_double()) || p.as_double() < 0.01 || p.as_double() > 1.) {
            result.successful = false;
            result.reason = "confidence_threshold must be a double in [0.01,1]";
            return result;
          }
          next = p.as_double();
        }
        confidence_ = next;
        return result;
      });
    }
    pub_ = create_publisher<Frame>(tracking ? "tracks" : "detections", rclcpp::SensorDataQoS());
    gst_init(nullptr, nullptr);
    // Separate process/pipeline per algorithm; tracker receives ROS pixels + injected boxes.
    std::string desc = "appsrc name=input is-live=true format=time block=false ! videoconvert ! video/x-raw,format=RGBA ! nvvideoconvert ! video/x-raw(memory:NVMM),format=NV12 ! mux.sink_0 nvstreammux name=mux batch-size=1 live-source=true width=" + std::to_string(width_) + " height=" + std::to_string(height_) + " batched-push-timeout=40000 ! " + (tracking ? "nvtracker" : "nvinfer") + " name=algorithm ! fakesink sync=false async=false";
    GError *error = nullptr;
    pipeline_ = gst_parse_launch(desc.c_str(), &error);
    if (error) {
      std::string message = error->message;
      g_error_free(error);
      if (pipeline_) gst_object_unref(pipeline_);
      pipeline_ = nullptr;
      throw std::runtime_error(message);
    }
    source_ = gst_bin_get_by_name(GST_BIN(pipeline_), "input");
    auto *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, width_, "height", G_TYPE_INT, height_, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
    gst_app_src_set_caps(GST_APP_SRC(source_), caps);
    gst_caps_unref(caps);
    auto *algorithm = gst_bin_get_by_name(GST_BIN(pipeline_), "algorithm");
    if (tracking) {
      g_object_set(algorithm, "ll-lib-file", lib.c_str(), "ll-config-file", config.c_str(), "tracker-width", 640, "tracker-height", 384, nullptr);
      auto *pad = gst_element_get_static_pad(algorithm, "sink");
      gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, inject, this, nullptr);
      gst_object_unref(pad);
    } else {
      g_object_set(algorithm, "config-file-path", config.c_str(), nullptr);
    }
    auto *pad = gst_element_get_static_pad(algorithm, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, extract, this, nullptr);
    gst_object_unref(pad);
    gst_object_unref(algorithm);
    bus_ = gst_element_get_bus(pipeline_);
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) throw std::runtime_error("DeepStream pipeline failed to start");
    timer_ = create_wall_timer(std::chrono::milliseconds(100), [this]() {
      auto *msg = gst_bus_pop_filtered(bus_, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
      if (!msg) return;
      if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = nullptr; gchar *debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        RCLCPP_FATAL(get_logger(), "%s (%s)", err->message, debug ? debug : "");
        g_error_free(err); g_free(debug);
      } else RCLCPP_ERROR(get_logger(), "Unexpected end of DeepStream stream");
      gst_message_unref(msg);
      rclcpp::shutdown();
    });
    if (tracking) frames_ = create_subscription<Frame>("detections", rclcpp::SensorDataQoS(), [this](Frame::ConstSharedPtr m) { push(*m); });
    else images_ = create_subscription<sensor_msgs::msg::Image>("camera/image_raw", rclcpp::SensorDataQoS(), [this](sensor_msgs::msg::Image::ConstSharedPtr m) { Frame f; f.image = *m; push(std::move(f)); });
  }
  ~Bridge() override {
    if (pipeline_) gst_element_set_state(pipeline_, GST_STATE_NULL);
    if (bus_) gst_object_unref(bus_);
    if (source_) gst_object_unref(source_);
    if (pipeline_) gst_object_unref(pipeline_);
  }
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  try { rclcpp::spin(std::make_shared<Bridge>()); }
  catch (const std::exception &e) { RCLCPP_FATAL(rclcpp::get_logger("deepstream"), "%s", e.what()); if (rclcpp::ok()) rclcpp::shutdown(); return 1; }
  if (rclcpp::ok()) rclcpp::shutdown();
  return 0;
}
