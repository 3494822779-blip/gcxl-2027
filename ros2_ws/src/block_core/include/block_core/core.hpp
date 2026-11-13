#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
namespace block_core {
struct Box {
  uint64_t track_id{};
  int class_id{};
  float confidence{}, x{}, y{}, width{}, height{};
  bool observed{true};
};
double iou(const Box&, const Box&);
class Lock {
 public:
  explicit Lock(double rebind_iou=.3, double timeout=3.);
  const Box* select(const std::vector<Box>&, double t=std::numeric_limits<double>::quiet_NaN(),
                    bool searching=false, double search_iou=-1.,
                    const std::set<uint64_t>& known_ids={}, bool stationary=true);
  void configure(double threshold,double timeout) { threshold_=threshold; timeout_=timeout; }
  void reset();
  uint64_t id() const { return id_; }
  bool locked() const { return has_id_; }
 private:
  uint64_t id_{};
  bool has_id_{}, has_last_{}, loss_active_{};
  Box last_{};
  std::optional<double> select_time_;
  double elapsed_{}, threshold_, timeout_;
  std::set<uint64_t> seen_, blocked_;
};
class CVFilter {
 public:
  std::vector<double> step(double t, const std::vector<double>* measurement,
                           double q, double r, double initial_variance);
  void reset();
  std::optional<double> time() const { return time_; }
  std::optional<double> last_seen() const { return last_seen_; }
 private:
  std::optional<double> time_, last_seen_;
  std::array<double,4> state_{};
  std::array<std::array<double,4>,4> covariance_{};
};
double rate(double error,double deadzone,double gain,double limit);
struct GripperSettings {
  double mm_per_px{.5}, claw_margin_mm{5}, link_ref_deg{60}, link_offset{44},
         link_length{91}, claw_init_deg{0}, claw_max_deg{90};
};
std::pair<double,double> gripper_angles(double,double,double,double,const GripperSettings&);
std::pair<double,double> wheel_speeds(double,double,double,double,double min_turn=0.);
struct Sweep {
  double elapsed{}, duration{}, tau{}, bias{};
  bool done() const { return elapsed>=duration; }
  void advance(double dt);
  double yaw() const;
};
std::vector<Box> candidates(const std::vector<Box>&, int target_class);
struct Obstacle { Box box; int side; double overlap; bool near; };
std::optional<Obstacle> obstacle_for(const Box&,const std::vector<Box>&);
void validate_finite(const std::vector<double>&);
uint16_t crc16(const uint8_t*,size_t);
std::string encode_command(uint16_t,double,double,double,double,double);
std::optional<std::pair<uint16_t,double>> decode_feedback(const std::string&);
}
