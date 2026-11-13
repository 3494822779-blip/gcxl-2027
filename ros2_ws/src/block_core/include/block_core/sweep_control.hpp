#pragma once
// Single-sided obstacle sweeping: admission budget, timed decaying yaw bias, claw restore and the
// post-sweep recovery gate. No ROS dependencies. `steady` is a monotonic clock in seconds and
// `stamp` is message/wall time; claw angles are only carried, never interpreted.
#include <map>
#include <optional>
#include <utility>
#include "block_core/core.hpp"
namespace block_core {
struct SweepSettings {
  double claw_deg{-30.}, overlap{.5}, weight{.2}, near_multiplier{2.}, max_yaw_rate{1.},
         tau_sec{.33}, duration_sec{1.}, settle_sec{.5};
  int max_attempts{2}, max_sweeps{8};
};
class SweepControl {
 public:
  struct Start { int side; double claw_deg; };   // side<0: left claw, side>0: right claw
  struct Tick { bool settling{}, finished{}; double bias{}; };
  void reset();
  bool active() const { return sweep_ && !sweep_->done(); }
  bool exists() const { return sweep_.has_value(); }
  // True when a new sweep may begin: none yet, or the last one finished and its recovery elapsed
  // and a newer observation than the recovery arrived.
  bool ready(double steady, double stamp, const SweepSettings&) const;
  // Admits one sweep against the per-obstacle and per-task budgets; nullopt means exhausted.
  std::optional<Start> begin(int side, double overlap, bool near, unsigned long obstacle_id,
                             double steady, std::pair<double,double> claws, const SweepSettings&);
  bool exhausted(unsigned long obstacle_id, const SweepSettings&) const;
  // Advance one control tick with valid control time `step`; on completion records the recovery.
  Tick tick(double steady, double step, double stamp, const SweepSettings&);
  bool recovering(double steady, const SweepSettings&) const;
  // Grasp must wait for an observation made after the sweep finished.
  bool awaiting_observation(double stamp) const { return sweep_ && stamp<=recovered_stamp_; }
  std::pair<double,double> restore() const { return restore_; }
  int count() const { return count_; }
  int attempts(unsigned long obstacle_id) const;
 private:
  std::optional<Sweep> sweep_;
  std::optional<double> recovered_;
  double started_{}, recovered_stamp_{};
  std::pair<double,double> restore_{};
  std::map<unsigned long,int> attempts_;
  int count_{};
};
}
