#pragma once
#include <block_interfaces/msg/target.hpp>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <string>

class TargetCache {
 public:
  using Target = block_interfaces::msg::Target;
  using Key = std::pair<int64_t, std::string>;
  static Key key(const std_msgs::msg::Header &h) {
    return {int64_t(h.stamp.sec) * 1000000000 + h.stamp.nanosec, h.frame_id};
  }
  void put(const Target &target) {
    std::lock_guard<std::mutex> lock(mutex_);
    targets_[key(target.header)] = target;
    while (targets_.size() > 64) targets_.erase(targets_.begin());
    changed_.notify_all();
  }
  std::optional<Target> wait(const std_msgs::msg::Header &header, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto wanted = key(header);
    changed_.wait_for(lock, timeout, [&] { return stopped_ || targets_.count(wanted); });
    auto found = targets_.find(wanted);
    if (stopped_ || found == targets_.end()) return std::nullopt;
    auto target = found->second;
    targets_.erase(targets_.begin(), std::next(found));
    return target;
  }
  void stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true; targets_.clear(); changed_.notify_all();
  }
 private:
  std::mutex mutex_;
  std::condition_variable changed_;
  std::map<Key, Target> targets_;
  bool stopped_ = false;
};
