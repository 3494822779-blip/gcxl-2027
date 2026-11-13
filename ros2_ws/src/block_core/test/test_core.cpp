#include <gtest/gtest.h>
#include "block_core/core.hpp"
using namespace block_core;
namespace {
Box box(uint64_t id,float x=100,float y=200,float w=300,float h=250,int cls=2) {
  return {id,cls,.9f,x,y,w,h,true};
}
}
TEST(Core, IoU) { auto a=box(1,0,0,2,2),b=box(2,1,0,2,2); EXPECT_DOUBLE_EQ(iou(a,b),1./3.); }
TEST(Lock, StableIdentityAndLoss) {
  Lock lock; std::vector<Box> b{box(1,0,0,20,100),box(2,0,0,20,140)};
  ASSERT_NE(lock.select(b),nullptr); EXPECT_EQ(lock.id(),2u);
  b[0].height=180; b[1].height=80; EXPECT_EQ(lock.select(b)->track_id,2u);
  b.erase(b.begin()+1); EXPECT_EQ(lock.select(b),nullptr); EXPECT_EQ(lock.id(),2u);
}
TEST(Lock, AllowsZeroId) { Lock l; std::vector<Box> b{box(0)}; ASSERT_NE(l.select(b,1),nullptr); EXPECT_TRUE(l.locked()); }
TEST(Lock, RebindOnFirstMissingFrame) {
  Lock l; std::vector<Box> b{box(5)}; l.select(b,1);
  b={box(6,105,205,295,245)}; ASSERT_NE(l.select(b,1.2),nullptr); EXPECT_EQ(l.id(),6u);
}
TEST(Lock, ExcludesPreviouslyObservedAndPredictedIds) {
  Lock l; auto predicted=box(3); predicted.observed=false;
  std::vector<Box> b{box(5),predicted}; l.select(b,1);
  b={box(3)}; EXPECT_EQ(l.select(b,1.2),nullptr);
  b.push_back(box(6)); ASSERT_NE(l.select(b,1.4),nullptr); EXPECT_EQ(l.id(),6u);
}
TEST(Lock, KnownOtherClassIdsCannotTakeOver) {
  Lock l; std::vector<Box> b{box(5)}; l.select(b,1,false,-1,{5,3});
  b={box(3)}; EXPECT_EQ(l.select(b,1.1),nullptr);
}
TEST(Lock, SearchPausesBudgetAndRequiresStationary) {
  Lock l(.3,.5); std::vector<Box> b{box(5)}; l.select(b,1);
  b={box(6)}; EXPECT_EQ(l.select(b,10,true,.1,{},false),nullptr);
  ASSERT_NE(l.select(b,11,true,.1,{},true),nullptr); EXPECT_EQ(l.id(),6u);
}
TEST(Lock, BudgetCountsIntervalsOnce) {
  Lock l(.3,1); std::vector<Box> b{box(5)}; l.select(b,1);
  l.select({},1.2); l.select({},1.4); b={box(6)};
  EXPECT_NE(l.select(b,1.8),nullptr);
}
TEST(Lock, ExpiryClassAndDisable) {
  Lock l; std::vector<Box> b{box(5)}; l.select(b,1);
  b={box(6,100,200,300,250,1)}; EXPECT_EQ(l.select(b,1.2),nullptr);
  b={box(7)}; EXPECT_EQ(l.select(b,5),nullptr);
  Lock off(0); b={box(5)}; off.select(b,1); b={box(6)}; EXPECT_EQ(off.select(b,1.2),nullptr);
}
TEST(Filter, VelocityPredictionAndOrdering) {
  CVFilter f;
  for(int i=0;i<100;++i) { double t=i/30.; std::vector<double> z{100+8*t,200-3*t}; f.step(t,&z,2,16,100); }
  auto predicted=f.step(3.5,nullptr,2,16,100);
  EXPECT_NEAR(predicted[0],128,.2); EXPECT_NEAR(predicted[1],189.5,.2);
  EXPECT_DOUBLE_EQ(*f.last_seen(),99/30.);
  EXPECT_TRUE(f.step(3,nullptr,2,16,100).empty());
  f.reset(); EXPECT_TRUE(f.step(4,nullptr,2,16,100).empty());
}
TEST(Filter, PythonGoldenJosephUpdate) {
  CVFilter f; std::vector<double> a{10,20},b{14,18};
  f.step(0,&a,2,16,100); auto out=f.step(1,&b,2,16,100);
  EXPECT_NEAR(out[0],13.704387990762125,1e-12);
  EXPECT_NEAR(out[1],18.14780600461894,1e-12);
  out=f.step(2,nullptr,2,16,100);
  EXPECT_NEAR(out[0],15.570438799076213,1e-12);
  EXPECT_NEAR(out[1],17.214780600461893,1e-12);
}
TEST(Geometry, ExplicitClassAndInvalidBoxes) {
  auto invalid=box(3); invalid.x=NAN;
  const auto c=candidates({box(1),box(2,0,0,20,20,1),invalid},2);
  ASSERT_EQ(c.size(),1u); EXPECT_EQ(c[0].track_id,1u);
}
TEST(Geometry, NearestObstacleFirst) {
  auto target=box(1,100,100,100,100);
  auto o=obstacle_for(target,{box(2,100,150,80,80),box(3,190,180,100,100)});
  ASSERT_TRUE(o); EXPECT_EQ(o->box.track_id,3u); EXPECT_EQ(o->side,1); EXPECT_NEAR(o->overlap,.1,1e-9); EXPECT_TRUE(o->near);
}
TEST(Gripper, PythonFormulaAndBounds) {
  GripperSettings s; auto a=gripper_angles(100,300,640,480,s);
  EXPECT_NEAR(a.first,std::acos((44-105./2)/91)-M_PI/3,1e-12);
  EXPECT_DOUBLE_EQ(a.first,a.second);
  EXPECT_TRUE(std::isnan(gripper_angles(300,100,640,480,s).first));
  EXPECT_GT(gripper_angles(100,400,640,480,s).first,a.first);
}
TEST(Control, SaturationMinimumTurnAndFinite) {
  EXPECT_EQ(rate(.02,.04,-.5,.3),0); EXPECT_EQ(rate(1,.04,-.5,.3),-.3);
  auto w=wheel_speeds(.2,.01,.2,.5,.1); EXPECT_NEAR(w.first,.1,1e-12); EXPECT_NEAR(w.second,.3,1e-12);
  EXPECT_THROW(wheel_speeds(NAN,0,.2,.5),std::invalid_argument);
}
TEST(Sweep, ValidTimeBudgetAndDecay) {
  Sweep s; s.duration=1; s.tau=.33; s.bias=.2;
  s.advance(-1); EXPECT_DOUBLE_EQ(s.elapsed,0); s.advance(.33); EXPECT_NEAR(s.yaw(),.2/std::exp(1),1e-12);
  s.advance(2); EXPECT_TRUE(s.done()); EXPECT_EQ(s.yaw(),0);
}
TEST(Serial, WireFormatAndInvalidFeedback) {
  EXPECT_EQ(encode_command(0,0,0,0,0,0),"$CMD,0,0.000,0.000,0.0,0.0,0.0*54");
  EXPECT_THROW(encode_command(0,NAN,0,0,0,0),std::invalid_argument);
  EXPECT_FALSE(decode_feedback("$FB,1,nan*00")); EXPECT_FALSE(decode_feedback("garbage"));
}
#include "block_core/task_memory.hpp"
#include "block_core/param_validate.hpp"
#include "block_core/tx_buffer.hpp"
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
TEST(Memory, SessionIsolationAndCopy) {
  TaskMemory a,b; EXPECT_NE(a.context()["session_id"],b.context()["session_id"]);
  auto copy=a.snapshot(); copy["context"]["held"]["state"]="wrong"; EXPECT_EQ(a.context()["held"]["state"],"unknown");
}
TEST(Memory, CommandedGraspNeverClaimsPhysicalConfirmation) {
  TaskMemory m; m.begin_task("target_grasp",7); m.target_class(2);
  const auto id=m.begin_grasp(7,false,5,2,0,5); EXPECT_EQ(m.begin_grasp(),id);
  m.phase("claw_close"); m.complete_grasp(); const auto rev=m.revision(); m.complete_grasp(); EXPECT_EQ(m.revision(),rev);
  EXPECT_EQ(m.context()["held"]["state"],"commanded_hold"); EXPECT_FALSE(m.context()["held"]["physical_confirmed"].asBool());
  EXPECT_TRUE(m.context()["task"]["first_formal_completed"].asBool());
  m.finish_task("completed","done"); EXPECT_EQ(m.context()["held"]["state"],"commanded_hold");
}
TEST(Memory, InterruptedSweepAndDiscard) {
  TaskMemory m; m.begin_task("target_grasp",1); m.begin_grasp(1,true,7,2); m.complete_grasp();
  m.begin_discard(); m.finish_task("canceled","turn_ccw");
  EXPECT_EQ(m.context()["latest_grasp"]["disposition"],"discard_interrupted"); EXPECT_EQ(m.context()["held"]["state"],"unknown");
  EXPECT_EQ(m.context()["unresolved_tasks"].size(),1u);
  m.begin_task("target_grasp",2); m.begin_sweep(7,2,9,-1); m.complete_sweep();
  EXPECT_EQ(m.context()["latest_sweep"]["side"],"left"); EXPECT_EQ(m.context()["latest_sweep"]["status"],"completed");
}
TEST(Parameters, RejectInvalidAndRelatedValues) {
  for(const auto* name:{"measurement_noise","bottom_target","deadzone_x","process_noise"}) {
    Json::Value p; p[name]=std::string(name)=="deadzone_x"?1.:std::string(name)=="process_noise"?NAN:0.;
    EXPECT_THROW(validate_parameters(p),std::invalid_argument);
  }
  Json::Value p; p["rebind_iou"]=.3; p["search_rebind_iou"]=.4; EXPECT_THROW(validate_parameters(p),std::invalid_argument);
  p["rebind_iou"]=0.; EXPECT_NO_THROW(validate_parameters(p));
}
TEST(TxBuffer, PartialWriteLatestOnlyAndResync) {
  TxBuffer tx(.1); std::string bytes;
  tx.enqueue("first\n",0); EXPECT_FALSE(tx.flush(0,[&](const auto& s){bytes+=s.substr(0,2); return 2;}));
  tx.enqueue("obsolete\n",.01); tx.enqueue("latest\n",.02);
  EXPECT_TRUE(tx.flush(.03,[&](const auto& s){bytes+=s;return s.size();}));
  EXPECT_TRUE(tx.flush(.04,[&](const auto& s){bytes+=s;return s.size();})); EXPECT_EQ(bytes,"first\nlatest\n");
  tx.enqueue("stale\n",.05); EXPECT_THROW(tx.flush(.2,[](const auto& s){return s.size();}),std::runtime_error);
  EXPECT_FALSE(tx.last_complete()); tx.enqueue("fresh\n",.21);
  tx.flush(.22,[&](const auto& s){EXPECT_EQ(s,"\nfresh\n");return s.size();});
}
TEST(TxBuffer, PseudoTerminalWireBytes) {
  int master=-1,slave=-1; ASSERT_EQ(openpty(&master,&slave,nullptr,nullptr,nullptr),0);
  termios attrs{}; ASSERT_EQ(tcgetattr(slave,&attrs),0); cfmakeraw(&attrs); ASSERT_EQ(tcsetattr(slave,TCSANOW,&attrs),0);
  TxBuffer tx(.1); tx.enqueue("$CMD,test\n",0);
  EXPECT_TRUE(tx.flush(.01,[&](const auto& s){return write(slave,s.data(),s.size());}));
  char buffer[64]{}; const auto n=read(master,buffer,sizeof(buffer)); EXPECT_EQ(std::string(buffer,n),"$CMD,test\n");
  close(master);close(slave);
}
#include "block_core/debug_log.hpp"
TEST(DebugLog, DisabledAndNamedSelection) {
  EXPECT_FALSE(DebugLog::enabled_for("actuator","off"));EXPECT_TRUE(DebugLog::enabled_for("actuator","1"));
  EXPECT_TRUE(DebugLog::enabled_for("actuator"," filter, actuator "));EXPECT_FALSE(DebugLog::enabled_for("gripper","actuator"));
}
TEST(DebugLog, JsonEventAndThrottle) {
  char directory[]="/tmp/gcxl-debug-test-XXXXXX";ASSERT_NE(mkdtemp(directory),nullptr);
  setenv("BP_DEBUG","actuator",1);setenv("BP_DEBUG_DIR",directory,1);
  DebugLog log("actuator");Json::Value fields;fields["state"]="track";
  log.event("state",2,fields,10);log.event("state",3,fields,10);
  std::ifstream input(std::string(directory)+"/actuator.jsonl");std::string first,second;ASSERT_TRUE(bool(std::getline(input,first)));EXPECT_FALSE(bool(std::getline(input,second)));
  Json::Value value;std::istringstream json(first);json>>value;EXPECT_EQ(value["state"],"track");EXPECT_DOUBLE_EQ(value["t"].asDouble(),2.);
  unsetenv("BP_DEBUG");unsetenv("BP_DEBUG_DIR");std::filesystem::remove_all(directory);
}
