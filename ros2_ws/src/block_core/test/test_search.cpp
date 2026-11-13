#include <gtest/gtest.h>
#include "block_core/search.hpp"
#include "block_core/core.hpp"
using namespace block_core;
namespace {
constexpr double kDt = .02;
SearchSettings settings() {
  SearchSettings s;
  s.track_width = .15; s.max_wheel_speed = .5; s.min_turn_speed = .17;
  s.turn_speed_mps = .08;  // deliberately below the motor threshold
  s.max_attempts = 2;
  return s;
}
// Runs one full pass; returns the number of ticks until completed() or the pass ends.
struct Pass { int ticks{}; bool wheels_moved{}; double min_cam{1e9}, max_cam{-1e9}; };
Pass run_pass(Search& search, const SearchSettings& s, double& cam, double& t) {
  Pass pass;
  const int before = search.attempts();
  for (int i = 0; i < 5000; ++i) {
    auto out = search.lost(t += kDt, kDt, cam, s);
    ++pass.ticks;
    pass.wheels_moved |= out.left != 0. || out.right != 0.;
    pass.min_cam = std::min(pass.min_cam, cam); pass.max_cam = std::max(pass.max_cam, cam);
    if (search.completed() || search.attempts() != before) break;
    if (search.direction() == 0) break;
  }
  return pass;
}
}  // namespace

TEST(Search, TurnMatchesTrackingYawSignAndMotorThreshold) {
  const auto s = settings();
  for (double yaw : {.4, -.4}) {
    Search search; search.set_side(yaw);
    double cam = 10., t = 0.;
    auto out = search.lost(t += kDt, kDt, cam, s);
    // Tracking: positive yaw slows the left wheel (left < right). Search must turn the same way.
    auto track = wheel_speeds(0, yaw, s.track_width, s.max_wheel_speed, s.min_turn_speed);
    EXPECT_EQ(out.left < out.right, track.first < track.second) << "yaw " << yaw;
    // The raw 0.08 m/s would not move the motors; the minimum turn speed must lift it.
    EXPECT_GE(std::abs(out.left), s.min_turn_speed - 1e-9);
    EXPECT_GE(std::abs(out.right), s.min_turn_speed - 1e-9);
    EXPECT_FALSE(out.stationary);
    EXPECT_DOUBLE_EQ(cam, 10.) << "camera must hold still while the chassis turns";
  }
}

TEST(Search, WithoutRememberedSideChassisStaysStopped) {
  const auto s = settings();
  Search search;  // no set_side(): nothing known about where the target left
  double cam = 10., t = 0.;
  auto out = search.lost(t += kDt, kDt, cam, s);
  EXPECT_EQ(out.left, 0.); EXPECT_EQ(out.right, 0.);
  EXPECT_GT(cam, 10.) << "falls back to the immediate pitch sweep";
}

TEST(Search, FinishesAfterOnePitchPass) {
  auto s = settings();
  Search search; search.set_side(.3);
  double cam = 40., t = 0.;
  bool went_down = false, came_back = false;
  for (int i = 0; i < 20000 && !search.completed(); ++i) {
    search.lost(t += kDt, kDt, cam, s);
    if (cam >= s.cam_max_deg) went_down = true;
    if (went_down && cam <= s.cam_min_deg) came_back = true;
  }
  EXPECT_TRUE(went_down); EXPECT_TRUE(came_back);
  EXPECT_TRUE(search.completed());
  EXPECT_EQ(search.direction(), 0);
  auto out = search.lost(t += kDt, kDt, cam, s);
  EXPECT_FALSE(out.active) << "an exhausted search must not move anything";
  EXPECT_EQ(out.left, 0.); EXPECT_EQ(out.right, 0.);
}

TEST(Search, AttemptsAreBoundedPerTaskAndResetByNewTask) {
  auto s = settings();
  s.cam_min_deg = 0.; s.cam_max_deg = 5.;  // short pitch range keeps the test fast
  Search search; search.set_side(.3);
  double cam = 2., t = 0.;
  // Each loss that ends by reacquisition uses one attempt of the budget.
  for (int k = 0; k < s.max_attempts; ++k) {
    search.lost(t += kDt, kDt, cam, s);
    EXPECT_EQ(search.attempts(), k + 1);
    search.reacquired();
    EXPECT_FALSE(search.completed());
  }
  search.lost(t += kDt, kDt, cam, s);
  EXPECT_TRUE(search.completed()) << "third loss in one task must not start another pass";
  EXPECT_EQ(search.attempts(), s.max_attempts);
  search.reset();
  EXPECT_FALSE(search.completed());
  search.set_side(.3);
  search.lost(t += kDt, kDt, cam, s);
  EXPECT_EQ(search.attempts(), 1) << "a new task starts with a fresh budget";
}

TEST(Search, ReacquisitionStopsSearchAndClearsExhaustion) {
  auto s = settings();
  Search search; search.set_side(.3);
  double cam = 40., t = 0.;
  search.lost(t += kDt, kDt, cam, s);
  EXPECT_NE(search.direction(), 0);
  search.reacquired();
  EXPECT_EQ(search.direction(), 0);
  EXPECT_FALSE(search.completed());
}
