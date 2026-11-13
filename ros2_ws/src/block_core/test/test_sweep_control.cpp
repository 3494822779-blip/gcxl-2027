#include <gtest/gtest.h>
#include <cmath>
#include "block_core/sweep_control.hpp"
using namespace block_core;
namespace {
SweepSettings settings() { SweepSettings s; s.duration_sec=1.; s.tau_sec=.33; s.settle_sec=.5; return s; }
// Drive a sweep to completion at 50 Hz; returns the steady time at which it finished.
double finish(SweepControl& c, const SweepSettings& s, double t, double stamp) {
  for (int i=0;i<2000;++i) { t+=.02; if (c.tick(t,.02,stamp,s).finished) return t; }
  ADD_FAILURE() << "sweep never finished"; return t;
}
}

TEST(SweepControl, PicksClawSideAndCarriesRestoreAngles) {
  SweepControl c; const auto s=settings();
  auto left=c.begin(-1,.9,false,7,0.,{25.,26.},s);
  ASSERT_TRUE(left);
  EXPECT_EQ(left->side,-1); EXPECT_DOUBLE_EQ(left->claw_deg,-30.);
  EXPECT_EQ(c.restore(), (std::pair<double,double>{25.,26.}));
  EXPECT_TRUE(c.active());
  SweepControl r;
  EXPECT_EQ(r.begin(1,.9,false,7,0.,{0.,0.},s)->side,1);
}

TEST(SweepControl, ClawSettlesBeforeAnyProgressThenBiasDecays) {
  SweepControl c; const auto s=settings();
  c.begin(1,.9,false,1,0.,{0,0},s);
  auto t0=c.tick(.2,.02,10.,s);
  EXPECT_TRUE(t0.settling);
  auto early=c.tick(.3,.02,10.,s);
  EXPECT_TRUE(early.settling) << "no progress while the claw is still moving in";
  EXPECT_DOUBLE_EQ(early.bias, s.weight*s.max_yaw_rate) << "bias starts undecayed";
  c.tick(.6,.02,10.,s);                         // settle elapsed, first counted step
  double prev=c.tick(.62,.02,10.,s).bias;
  for (double t=.64; t<1.4; t+=.02) { double b=c.tick(t,.02,10.,s).bias; if (b) { EXPECT_LT(b,prev); prev=b; } }
  EXPECT_GT(prev, 0.) << "right-side sweep biases positive";
}

TEST(SweepControl, BiasOnlyWhenOverlapStrictlyExceedsThresholdAndNearScales) {
  const auto s=settings();
  // Sampled inside the claw settle window, so the bias has not started to decay.
  SweepControl exact; exact.begin(-1,s.overlap,false,1,0.,{0,0},s);
  EXPECT_DOUBLE_EQ(exact.tick(.2,.02,1.,s).bias,0.) << "exactly at the threshold is not enough";
  SweepControl over; over.begin(-1,s.overlap+.01,false,1,0.,{0,0},s);
  EXPECT_DOUBLE_EQ(over.tick(.2,.02,1.,s).bias, -s.weight*s.max_yaw_rate);
  SweepControl near; near.begin(-1,.9,true,1,0.,{0,0},s);
  EXPECT_DOUBLE_EQ(near.tick(.2,.02,1.,s).bias, -s.weight*s.near_multiplier*s.max_yaw_rate);
}

TEST(SweepControl, FinishRecordsRecoveryAndRequiresNewerObservation) {
  SweepControl c; const auto s=settings();
  c.begin(1,.9,false,1,0.,{3.,4.},s);
  const double done=finish(c,s,0.,/*stamp*/100.);
  EXPECT_FALSE(c.active()); EXPECT_TRUE(c.exists());
  EXPECT_TRUE(c.recovering(done+.1,s)) << "chassis stays stopped while claws return";
  EXPECT_FALSE(c.recovering(done+s.settle_sec+.01,s));
  EXPECT_FALSE(c.ready(done+.1,101.,s)) << "recovery time has not elapsed";
  EXPECT_FALSE(c.ready(done+1.,100.,s)) << "same-stamp observation predates the recovery";
  EXPECT_TRUE(c.ready(done+1.,100.01,s));
  EXPECT_TRUE(c.awaiting_observation(100.)); EXPECT_FALSE(c.awaiting_observation(100.01));
  EXPECT_EQ(c.restore(), (std::pair<double,double>{3.,4.}));
}

TEST(SweepControl, NothingStartedYetIsReadyButHasNoObservationWait) {
  SweepControl c; const auto s=settings();
  EXPECT_TRUE(c.ready(0.,0.,s)); EXPECT_FALSE(c.awaiting_observation(0.));
  c.begin(1,.9,false,1,0.,{0,0},s);
  EXPECT_FALSE(c.ready(.1,1.,s)) << "a sweep in progress blocks the next one";
}

TEST(SweepControl, PerObstacleAndPerTaskBudgetsStopFurtherSweeps) {
  auto s=settings(); s.max_attempts=2; s.max_sweeps=3;
  SweepControl c;
  ASSERT_TRUE(c.begin(1,.9,false,10,0.,{0,0},s)); finish(c,s,0.,1.);
  ASSERT_TRUE(c.begin(1,.9,false,10,5.,{0,0},s)); finish(c,s,5.,2.);
  EXPECT_TRUE(c.exhausted(10,s)); EXPECT_FALSE(c.begin(1,.9,false,10,9.,{0,0},s));
  EXPECT_EQ(c.attempts(10),2) << "a refused sweep is not counted";
  ASSERT_TRUE(c.begin(1,.9,false,11,10.,{0,0},s)) << "another obstacle has its own allowance";
  finish(c,s,10.,3.);
  EXPECT_EQ(c.count(),3);
  EXPECT_TRUE(c.exhausted(12,s)) << "task total reached";
  EXPECT_FALSE(c.begin(1,.9,false,12,20.,{0,0},s));
}

TEST(SweepControl, ResetForNewTaskForgetsEverything) {
  auto s=settings(); s.max_attempts=1;
  SweepControl c; c.begin(1,.9,false,5,0.,{0,0},s); finish(c,s,0.,1.);
  EXPECT_TRUE(c.exhausted(5,s));
  c.reset();
  EXPECT_FALSE(c.exists()); EXPECT_EQ(c.count(),0); EXPECT_FALSE(c.exhausted(5,s));
  EXPECT_TRUE(c.ready(0.,0.,s));
}
