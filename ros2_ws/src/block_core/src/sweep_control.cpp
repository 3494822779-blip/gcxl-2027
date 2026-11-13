#include "block_core/sweep_control.hpp"
#include <algorithm>
namespace block_core {
void SweepControl::reset() {
  sweep_.reset(); recovered_.reset(); attempts_.clear();
  started_=recovered_stamp_=0; restore_={}; count_=0;
}
int SweepControl::attempts(unsigned long id) const {
  auto it=attempts_.find(id); return it==attempts_.end()?0:it->second;
}
bool SweepControl::exhausted(unsigned long id,const SweepSettings& s) const {
  return attempts(id)>=s.max_attempts || count_>=s.max_sweeps;
}
bool SweepControl::ready(double steady,double stamp,const SweepSettings& s) const {
  return !sweep_ || (sweep_->done() && recovered_ && steady-*recovered_>=s.settle_sec && stamp>recovered_stamp_);
}
std::optional<SweepControl::Start> SweepControl::begin(int side,double overlap,bool near,unsigned long id,
    double steady,std::pair<double,double> claws,const SweepSettings& s) {
  if(exhausted(id,s)) return std::nullopt;
  ++attempts_[id]; ++count_; started_=steady; recovered_.reset(); restore_=claws;
  Sweep w; w.duration=s.duration_sec; w.tau=s.tau_sec;
  const double weight=s.weight*(near?s.near_multiplier:1.);
  w.bias=overlap>s.overlap?side*std::min(1.,weight)*s.max_yaw_rate:0.;
  sweep_=w;
  return Start{side<0?-1:1,s.claw_deg};
}
SweepControl::Tick SweepControl::tick(double steady,double step,double stamp,const SweepSettings& s) {
  Tick out;
  if(!active()) return out;
  out.settling=steady-started_<s.settle_sec;
  if(!out.settling) sweep_->advance(step);
  out.bias=sweep_->yaw();
  if(sweep_->done()) { out.finished=true; recovered_=steady; recovered_stamp_=stamp; }
  return out;
}
bool SweepControl::recovering(double steady,const SweepSettings& s) const {
  return recovered_ && steady-*recovered_<s.settle_sec;
}
}
