#include "block_core/task_memory.hpp"
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
namespace block_core {
namespace {
std::string identifier() {
  static std::mt19937_64 random(std::random_device{}()); std::ostringstream out;
  out<<std::hex<<std::setfill('0')<<std::setw(16)<<random()<<std::setw(16)<<random(); return out.str();
}
std::string timestamp() {
  const auto now=std::chrono::system_clock::now(); const auto t=std::chrono::system_clock::to_time_t(now);
  std::tm utc{}; gmtime_r(&t,&utc); std::ostringstream out; out<<std::put_time(&utc,"%Y-%m-%dT%H:%M:%SZ"); return out.str();
}
}
TaskMemory::TaskMemory():session_(identifier()) {
  data_["version"]=1; for(const auto* kind:{"tasks","grasps","discards","sweeps"}) data_[kind]=Json::Value(Json::arrayValue);
  held_["state"]="unknown"; held_["grasp_id"]=Json::nullValue; held_["physical_confirmed"]=false;
}
Json::Value* TaskMemory::current(const std::string& k) { const auto it=current_.find(k); return it==current_.end()?nullptr:&data_[k][it->second]; }
const Json::Value* TaskMemory::current(const std::string& k) const { const auto it=current_.find(k); return it==current_.end()?nullptr:&data_[k][it->second]; }
Json::Value& TaskMemory::record(const std::string& k) {
  Json::Value r; r["id"]=identifier(); r["session_id"]=session_; r["started_at"]=timestamp(); r["status"]="running";
  data_[k].append(r); current_[k]=data_[k].size()-1; return *current(k);
}
void TaskMemory::begin_task(const std::string& operation,uint64_t first_id) {
  auto& task=record("tasks"); task["operation"]=operation; task["first_grasp_id"]=Json::UInt64(first_id);
  task["phase"]="starting"; task["first_formal_completed"]=false; task["clearance_count"]=0;
  for(const auto* kind:{"grasps","discards","sweeps"}) current_.erase(kind);
  ++revision_;
}
void TaskMemory::target_class(int cls) { if(auto* task=current("tasks")) { (*task)["target_class_id"]=cls; ++revision_; } }
void TaskMemory::begin_sweep(uint64_t track,int cls,uint64_t obstacle,int side) {
  uncertain_hold("sweep_claw_command"); auto& r=record("sweeps"); const auto* task=current("tasks");
  r["task_id"]=task?(*task)["id"]:Json::Value(); r["track_id"]=Json::UInt64(track); r["class_id"]=cls;
  r["obstacle_id"]=Json::UInt64(obstacle); r["side"]=side<0?"left":"right"; r["phase"]="sweeping"; r["physical_confirmed"]=false; ++revision_;
}
void TaskMemory::complete_sweep() { auto* r=current("sweeps"); if(r && (*r)["status"]=="running") { (*r)["status"]="completed"; (*r)["finished_at"]=timestamp(); ++revision_; } }
std::string TaskMemory::begin_grasp(uint64_t first_id,bool clearance,std::optional<uint64_t> track,std::optional<int> cls,double left,double right) {
  auto* old=current("grasps"); if(old && (*old)["status"]=="running") return (*old)["id"].asString();
  uncertain_hold("new_grasp"); auto& r=record("grasps"); const auto* task=current("tasks");
  r["task_id"]=task?(*task)["id"]:Json::Value(); r["first_grasp_id"]=Json::UInt64(first_id);
  r["purpose"]=first_id?(clearance?"clearance":"first_formal"):"normal";
  r["track_id"]=track?Json::Value(Json::UInt64(*track)):Json::Value(); r["class_id"]=cls?Json::Value(*cls):Json::Value();
  r["left_close_deg"]=left; r["right_close_deg"]=right; r["physical_confirmed"]=false; r["phase"]="grasp_pitch"; r["disposition"]="pending"; ++revision_; return r["id"].asString();
}
void TaskMemory::phase(const std::string& phase) {
  auto* discard=current("discards"); bool changed=false;
  for(auto* r:{current("tasks"),discard && (*discard)["status"]=="running"?discard:current("grasps")})
    if(r && (*r)["status"]=="running" && (*r)["phase"]!=phase) { (*r)["phase"]=phase; changed=true; }
  if(changed) ++revision_;
}
void TaskMemory::complete_grasp() {
  auto* r=current("grasps"); if(!r || (*r)["status"]!="running") return;
  (*r)["status"]="completed"; (*r)["finished_at"]=timestamp(); (*r)["disposition"]="commanded_hold";
  held_=Json::Value(); held_["state"]="commanded_hold"; held_["grasp_id"]=(*r)["id"]; held_["physical_confirmed"]=false;
  if(auto* task=current("tasks")) if((*r)["purpose"]=="first_formal") (*task)["first_formal_completed"]=true;
  ++revision_;
}
void TaskMemory::begin_discard() {
  auto* old=current("discards"); if(old && (*old)["status"]=="running") return;
  const auto linked=held_["state"]=="commanded_hold"?held_["grasp_id"]:Json::Value();
  auto& r=record("discards"); const auto* task=current("tasks"); r["task_id"]=task?(*task)["id"]:Json::Value();
  r["grasp_id"]=linked; r["phase"]="turn_ccw"; r["physical_confirmed"]=false;
  for(auto& g:data_["grasps"]) if(g["id"]==linked) g["disposition"]="discard_running";
  ++revision_;
}
void TaskMemory::complete_discard() {
  auto* r=current("discards"); if(!r || (*r)["status"]!="running") return;
  (*r)["status"]="completed"; (*r)["finished_at"]=timestamp();
  for(auto& g:data_["grasps"]) if(g["id"]==(*r)["grasp_id"]) {
    g["disposition"]="discard_completed";
    if(auto* task=current("tasks")) if(g["task_id"]==(*task)["id"] && g["purpose"]=="clearance") (*task)["clearance_count"]=(*task)["clearance_count"].asInt()+1;
  }
  held_=Json::Value(); held_["state"]="release_command_completed"; held_["grasp_id"]=Json::nullValue; held_["physical_confirmed"]=false; ++revision_;
}
void TaskMemory::uncertain_hold(const std::string& reason) {
  if(held_["state"]=="unknown") return;
  const auto linked=held_["grasp_id"]; for(auto& g:data_["grasps"]) if(g["id"]==linked) g["disposition"]="unknown";
  held_=Json::Value(); held_["state"]="unknown"; held_["grasp_id"]=linked; held_["reason"]=reason; held_["physical_confirmed"]=false; ++revision_;
}
void TaskMemory::finish_task(const std::string& status,const std::string& phase,const std::string& reason) {
  auto* task=current("tasks"); if(!task || (*task)["status"]!="running") return;
  (*task)["status"]=status; (*task)["phase"]=phase; (*task)["reason"]=reason; (*task)["finished_at"]=timestamp();
  bool interrupted=false;
  for(const auto* kind:{"grasps","discards","sweeps"}) if(auto* r=current(kind)) if((*r)["status"]=="running") {
    (*r)["status"]=status=="completed"?"interrupted":status; (*r)["phase"]=phase; (*r)["reason"]=reason; (*r)["finished_at"]=timestamp(); interrupted=true;
  }
  if(auto* discard=current("discards")) if((*discard)["status"]!="completed")
    for(auto& g:data_["grasps"]) if(g["id"]==(*discard)["grasp_id"]) g["disposition"]="discard_interrupted";
  if(interrupted) { auto* g=current("grasps"); auto linked=g?(*g)["id"]:held_["grasp_id"]; held_=Json::Value(); held_["state"]="unknown"; held_["grasp_id"]=linked; held_["physical_confirmed"]=false; }
  ++revision_;
}
Json::Value TaskMemory::context() const {
  Json::Value out; out["version"]=1; out["session_id"]=session_; out["revision"]=Json::UInt64(revision_);
  const auto* task=current("tasks"); out["task"]=task?*task:Json::Value(); out["held"]=held_;
  for(const auto& pair:std::map<std::string,std::string>{{"sweeps","latest_sweep"},{"grasps","latest_grasp"},{"discards","latest_discard"}}) {
    const auto& list=data_[pair.first]; out[pair.second]=list.empty()?Json::Value():list[list.size()-1];
  }
  out["unresolved_tasks"]=Json::Value(Json::arrayValue);
  for(const auto& t:data_["tasks"]) { const auto status=t["status"].asString(); if(status=="canceled" || status=="timeout" || status=="aborted" || status=="interrupted") out["unresolved_tasks"].append(t); }
  return out;
}
Json::Value TaskMemory::snapshot() const { Json::Value out; out["context"]=context(); out["history"]=data_; return out; }
std::string TaskMemory::encode(const Json::Value& value) { Json::StreamWriterBuilder builder; builder["indentation"]=""; return Json::writeString(builder,value); }
}
