#pragma once
#include <cmath>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
namespace block_core {
// The writer returns -1 for EAGAIN and throws on other errors.
class TxBuffer {
 public:
  explicit TxBuffer(double timeout):timeout_(timeout) {
    if(!std::isfinite(timeout) || timeout<=0) throw std::invalid_argument("write timeout must be positive and finite");
  }
  void enqueue(std::string bytes,double now) { latest_=std::move(bytes); latest_at_=now; }
  void fail() { pending_.clear(); latest_.reset(); last_complete_.reset(); resync_=true; }
  bool flush(double now,const std::function<long(const std::string&)>& write) {
    try {
      if(pending_.empty() && latest_) {
        if(now-latest_at_>=timeout_) throw std::runtime_error("queued serial command expired");
        pending_=(resync_?"\n":"")+*latest_; latest_.reset(); started_=latest_at_;
      }
      if(pending_.empty()) return false;
      if(now-started_>=timeout_) throw std::runtime_error("serial frame write timed out");
      const auto count=write(pending_);
      if(count==-1) return false;
      if(count<=0 || static_cast<size_t>(count)>pending_.size()) throw std::runtime_error("serial write made no progress");
      pending_.erase(0,count);
      if(pending_.empty()) { resync_=false; last_complete_=now; return true; }
      return false;
    } catch(...) { fail(); throw; }
  }
  std::optional<double> last_complete() const { return last_complete_; }
 private:
  double timeout_,latest_at_{},started_{};
  bool resync_{}; std::string pending_; std::optional<std::string> latest_;
  std::optional<double> last_complete_;
};
}
