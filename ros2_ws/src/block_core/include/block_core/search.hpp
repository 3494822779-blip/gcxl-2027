#pragma once
// Lost-target search: turn toward the side where the target was last seen, pause so the image
// settles (lets the selector rebind with a comparable frame), then sweep the camera pitch down
// and back up once. At most max_attempts passes per task. No ROS dependencies; times in seconds.
namespace block_core {
struct SearchSettings {
  double turn_sec{.35}, dwell_sec{.15}, turn_speed_mps{.17}, pitch_speed_deg{20.};
  double cam_min_deg{0.}, cam_max_deg{90.};
  double track_width{.15}, max_wheel_speed{.5}, min_turn_speed{0.};
  int max_attempts{3};
};
class Search {
 public:
  struct Output {
    double left{}, right{};     // wheel speeds in m/s; zero while pausing or sweeping the camera
    bool active{}, stationary{true};  // published so the selector knows when rebinding is safe
  };
  void reset();                 // new task: forget attempts and the remembered side
  void set_side(double yaw_rate);  // sign of the tracking yaw command = side the target left on
  // One control tick while the target is lost. camera_deg is advanced in place.
  Output lost(double t, double dt, double& camera_deg, const SearchSettings&);
  void reacquired();            // real observation: end this pass, next loss gets a fresh pass
  void stop();                  // leave the search without ending the budget
  int direction() const { return direction_; }  // +1 pitching down, -1 back up, 0 idle
  bool completed() const { return completed_; }
  int attempts() const { return attempts_; }
 private:
  int direction_{}, attempts_{};
  bool completed_{};
  double side_{}, turn_{}, dwell_{}, time_{};
};
}
