#pragma once
// "Open once" claw opening for one target: the opening is measured a single time from the visual
// proposal and then held for the whole task, across width jitter, pitch crossings, sweeps and
// tracker-ID rebinding. No ROS dependencies; `t` is a monotonic clock in seconds, angles in degrees.
#include <cstdint>
#include <optional>
#include <utility>
namespace block_core {
class ClawOpening {
 public:
  using Angles = std::pair<double,double>;
  void reset();
  bool has_angles() const { return angles_.has_value(); }
  bool opened() const { return opened_.has_value(); }
  const std::optional<Angles>& angles() const { return angles_; }
  // Another target than the saved one: the opening belongs to the old block, drop it.
  void target_changed(uint64_t track);
  // The tracker re-created the same block under a new ID: keep the saved opening for it.
  void follow(uint64_t new_track);
  // Saves the proposal once. `trigger` is the pitch/open condition; nullopt proposal = not usable now.
  bool capture(uint64_t track, bool trigger, std::optional<Angles> proposal, double lo, double hi);
  // Commands the saved opening for this target and starts the settle timer; nullopt if none.
  std::optional<Angles> apply(uint64_t track, double t);
  void invalidate_timer() { opened_.reset(); }              // a sweep overrides one claw
  void restored(double t) { opened_ = angles_ ? std::optional<double>(t) : std::nullopt; }
  bool settling(double t, double settle) const { return opened_ && t-*opened_<settle; }
  // True while the grasp must still wait for opening (no saved angles for this target, or timer unset).
  bool needs_open(uint64_t track) const { return !track_ || *track_!=track || !angles_; }
 private:
  std::optional<Angles> angles_;
  std::optional<uint64_t> track_;
  std::optional<double> opened_;
};
}
