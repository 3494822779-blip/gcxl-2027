#pragma once
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

inline int64_t monotonic_ns() {
  timespec t{};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return int64_t(t.tv_sec) * 1000000000 + t.tv_nsec;
}

class V4l2Source {
  int fd_ = -1;
  bool streaming_ = false;
  struct Mapping { void *data; size_t length; };
  std::vector<Mapping> mappings_;
  bool have_sequence_ = false;
  uint32_t last_sequence_ = 0;
  static bool automatic(uint32_t id) {
    return id == V4L2_CID_AUTO_WHITE_BALANCE || id == V4L2_CID_AUTOGAIN ||
           id == V4L2_CID_HUE_AUTO || id == V4L2_CID_EXPOSURE_AUTO ||
           id == V4L2_CID_FOCUS_AUTO || id == V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
  }
  void call(unsigned long op, void *arg) {
    int result;
    do { result = ioctl(fd_, op, arg); } while (result < 0 && errno == EINTR);
    if (result < 0) throw std::runtime_error("V4L2 ioctl " + std::to_string(op) + ": " + strerror(errno));
  }
  void defaults(const std::function<void(const std::string &)> &log) {
    std::vector<v4l2_queryctrl> controls;
    v4l2_queryctrl q{};
    q.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (ioctl(fd_, VIDIOC_QUERYCTRL, &q) == 0) {
      auto cls = V4L2_CTRL_ID2CLASS(q.id);
      // Never execute button/relative-movement controls. Reset image controls only.
      bool motion = q.id == V4L2_CID_PAN_RELATIVE || q.id == V4L2_CID_TILT_RELATIVE ||
                    q.id == V4L2_CID_PAN_ABSOLUTE || q.id == V4L2_CID_TILT_ABSOLUTE ||
                    q.id == V4L2_CID_ZOOM_RELATIVE || q.id == V4L2_CID_ZOOM_CONTINUOUS ||
                    q.id == V4L2_CID_PAN_SPEED || q.id == V4L2_CID_TILT_SPEED;
      if ((cls == V4L2_CTRL_CLASS_USER || cls == V4L2_CTRL_CLASS_CAMERA) && !motion &&
          (q.type == V4L2_CTRL_TYPE_INTEGER || q.type == V4L2_CTRL_TYPE_BOOLEAN ||
           q.type == V4L2_CTRL_TYPE_MENU || q.type == V4L2_CTRL_TYPE_INTEGER_MENU)) controls.push_back(q);
      q.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
    if (errno != EINVAL) throw std::runtime_error("Cannot enumerate V4L2 controls");
    for (bool auto_pass : {true, false}) for (const auto &original : controls) {
      if (automatic(original.id) != auto_pass) continue;
      q = {}; q.id = original.id;
      call(VIDIOC_QUERYCTRL, &q);
      if (q.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_GRABBED | V4L2_CTRL_FLAG_READ_ONLY |
                     V4L2_CTRL_FLAG_INACTIVE | V4L2_CTRL_FLAG_WRITE_ONLY | V4L2_CTRL_FLAG_VOLATILE)) continue;
      v4l2_control c{}; c.id = q.id; c.value = q.default_value;
      try {
        call(VIDIOC_S_CTRL, &c);
        call(VIDIOC_G_CTRL, &c);
        if (c.value != q.default_value) throw std::runtime_error("readback differs from default");
      } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Cannot restore ") + reinterpret_cast<char *>(q.name) + ": " + e.what());
      }
      log(std::string(reinterpret_cast<char *>(q.name)) + "=" + std::to_string(c.value));
    }
  }
 public:
  struct Frame { std::vector<uint8_t> jpeg; int64_t timestamp_ns = 0; };
  uint64_t sequence_gaps = 0, error_frames = 0;
  V4l2Source(const std::string &path, int width, int height, int fps,
            const std::function<void(const std::string &)> &log) {
    try {
      fd_ = open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
      if (fd_ < 0) throw std::runtime_error("Cannot open " + path + ": " + strerror(errno));
      v4l2_format format{}; format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      format.fmt.pix.width = width; format.fmt.pix.height = height;
      format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
      call(VIDIOC_S_FMT, &format);
      if (format.fmt.pix.width != unsigned(width) || format.fmt.pix.height != unsigned(height) ||
          format.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG)
        throw std::runtime_error("Camera did not accept configured MJPEG dimensions");
      v4l2_streamparm parm{}; parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      parm.parm.capture.timeperframe = {1, unsigned(fps)};
      call(VIDIOC_S_PARM, &parm);
      log("negotiated fps=" + std::to_string(parm.parm.capture.timeperframe.denominator) + "/" +
          std::to_string(parm.parm.capture.timeperframe.numerator));
      defaults(log);
      v4l2_requestbuffers req{}; req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      req.memory = V4L2_MEMORY_MMAP; req.count = 4;
      call(VIDIOC_REQBUFS, &req);
      if (req.count < 2) throw std::runtime_error("Not enough V4L2 mmap buffers");
      for (unsigned i = 0; i < req.count; ++i) {
        v4l2_buffer b{}; b.type = req.type; b.memory = req.memory; b.index = i;
        call(VIDIOC_QUERYBUF, &b);
        auto data = mmap(nullptr, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, b.m.offset);
        if (data == MAP_FAILED) throw std::runtime_error("V4L2 mmap failed");
        mappings_.push_back({data, b.length});
        call(VIDIOC_QBUF, &b);
      }
      int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      call(VIDIOC_STREAMON, &type); streaming_ = true;
    } catch (...) { close(); throw; }
  }
  V4l2Source(const V4l2Source &) = delete;
  V4l2Source &operator=(const V4l2Source &) = delete;
  ~V4l2Source() { close(); }
  bool read(Frame &out) {
    pollfd p{fd_, POLLIN, 0};
    int result;
    do { result = poll(&p, 1, 500); } while (result < 0 && errno == EINTR);
    if (result == 0) return false;
    if (result < 0 || (p.revents & (POLLERR | POLLHUP | POLLNVAL))) throw std::runtime_error("Camera disconnected or poll failed");
    v4l2_buffer b{}; b.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; b.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_, VIDIOC_DQBUF, &b) < 0) {
      if (errno == EAGAIN || errno == EINTR) return false;
      throw std::runtime_error(std::string("V4L2 dequeue: ") + strerror(errno));
    }
    try {
      uint32_t delta = b.sequence - last_sequence_;
      if (have_sequence_ && delta > 1 && delta < 0x80000000) sequence_gaps += delta - 1;
      have_sequence_ = true; last_sequence_ = b.sequence;
      if (b.index >= mappings_.size() || b.bytesused > mappings_[b.index].length)
        throw std::runtime_error("Invalid V4L2 buffer bounds");
      if ((b.flags & V4L2_BUF_FLAG_ERROR) || b.bytesused == 0) {
        ++error_frames; call(VIDIOC_QBUF, &b); return false;
      }
      auto data = static_cast<uint8_t *>(mappings_[b.index].data);
      out.jpeg.assign(data, data + b.bytesused);
      out.timestamp_ns = monotonic_ns();
      if ((b.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) == V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC)
        out.timestamp_ns = int64_t(b.timestamp.tv_sec) * 1000000000 + b.timestamp.tv_usec * 1000;
    } catch (...) { ioctl(fd_, VIDIOC_QBUF, &b); throw; }
    call(VIDIOC_QBUF, &b);
    return true;
  }
  void close() noexcept {
    if (fd_ < 0) return;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (streaming_) ioctl(fd_, VIDIOC_STREAMOFF, &type);
    streaming_ = false;
    for (auto m : mappings_) munmap(m.data, m.length);
    mappings_.clear(); ::close(fd_); fd_ = -1;
  }
};
