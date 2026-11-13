#include <gtest/gtest.h>
#include "block_core/claw_opening.hpp"
using namespace block_core;
using Angles = ClawOpening::Angles;

TEST(ClawOpening, MeasuredOnceAndNeverRecomputedFromLaterWidth) {
  ClawOpening o;
  EXPECT_TRUE(o.capture(7,true,Angles{30,32},0,90));
  EXPECT_FALSE(o.capture(7,true,Angles{60,60},0,90)) << "width changed after approach: keep the first opening";
  EXPECT_EQ(*o.angles(), (Angles{30,32}));
}

TEST(ClawOpening, NeedsTriggerAndAUsableProposal) {
  ClawOpening o;
  EXPECT_FALSE(o.capture(7,false,Angles{30,30},0,90)) << "camera not yet at the open pitch";
  EXPECT_FALSE(o.capture(7,true,std::nullopt,0,90)) << "no fresh matching gripper proposal";
  EXPECT_FALSE(o.has_angles());
  EXPECT_TRUE(o.needs_open(7));
}

TEST(ClawOpening, ProposalIsClampedToClawLimits) {
  ClawOpening o;
  o.capture(1,true,Angles{-5,200},0,90);
  EXPECT_EQ(*o.angles(), (Angles{0,90}));
}

TEST(ClawOpening, ApplyStartsSettleTimerOnlyForTheSavedTarget) {
  ClawOpening o; o.capture(7,true,Angles{30,30},0,90);
  EXPECT_FALSE(o.apply(8,1.)) << "never command another block's opening";
  EXPECT_FALSE(o.opened());
  ASSERT_TRUE(o.apply(7,2.));
  EXPECT_TRUE(o.opened());
  EXPECT_TRUE(o.settling(2.3,.5)); EXPECT_FALSE(o.settling(2.5,.5));
  EXPECT_FALSE(o.needs_open(7));
}

TEST(ClawOpening, DifferentTargetDropsTheOpening) {
  ClawOpening o; o.capture(7,true,Angles{30,30},0,90); o.apply(7,1.);
  o.target_changed(7);
  EXPECT_TRUE(o.has_angles()) << "same target keeps it";
  o.target_changed(9);
  EXPECT_FALSE(o.has_angles()); EXPECT_FALSE(o.opened());
  EXPECT_TRUE(o.capture(9,true,Angles{40,40},0,90)) << "a new target may be measured again";
}

TEST(ClawOpening, TrackerRebindKeepsSavedOpeningWithoutRemeasuring) {
  ClawOpening o; o.capture(7,true,Angles{30,31},0,90); o.apply(7,1.);
  o.follow(12);
  o.target_changed(12);
  ASSERT_TRUE(o.has_angles());
  EXPECT_FALSE(o.needs_open(12));
  EXPECT_FALSE(o.capture(12,true,Angles{70,70},0,90));
  EXPECT_EQ(*o.apply(12,2.), (Angles{30,31}));
}

TEST(ClawOpening, SweepRestoreReusesSavedOpeningAndRestartsSettle) {
  ClawOpening o; o.capture(7,true,Angles{30,30},0,90); o.apply(7,1.);
  o.invalidate_timer();
  EXPECT_FALSE(o.opened());
  o.restored(5.);
  EXPECT_TRUE(o.opened()); EXPECT_TRUE(o.settling(5.2,.5));
  ClawOpening none; none.restored(5.);
  EXPECT_FALSE(none.opened()) << "nothing saved yet, nothing to wait for";
}

TEST(ClawOpening, ResetForNextTask) {
  ClawOpening o; o.capture(7,true,Angles{30,30},0,90); o.apply(7,1.);
  o.reset();
  EXPECT_FALSE(o.has_angles()); EXPECT_FALSE(o.opened()); EXPECT_TRUE(o.needs_open(7));
}
