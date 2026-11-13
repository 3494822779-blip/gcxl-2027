#pragma once
#include <json/json.h>
#include <optional>
#include <map>
#include <cstdint>
#include <string>
namespace block_core {
class TaskMemory {
 public:
  TaskMemory();
  void begin_task(const std::string&,uint64_t first_id=0);
  void target_class(int cls);
  void begin_sweep(uint64_t,int,uint64_t,int);
  void complete_sweep();
  std::string begin_grasp(uint64_t first_id=0,bool clearance=false,
    std::optional<uint64_t> track={},std::optional<int> cls={},double left=0,double right=0);
  void phase(const std::string&);
  void complete_grasp();
  void begin_discard();
  void complete_discard();
  void uncertain_hold(const std::string&);
  void finish_task(const std::string&,const std::string&,const std::string& reason="");
  Json::Value context() const;
  Json::Value snapshot() const;
  static std::string encode(const Json::Value&);
  uint64_t revision() const { return revision_; }
 private:
  Json::Value& record(const std::string&);
  Json::Value* current(const std::string&);
  const Json::Value* current(const std::string&) const;
  Json::Value data_,held_;
  std::map<std::string,Json::ArrayIndex> current_;
  std::string session_;
  uint64_t revision_{};
};
}
