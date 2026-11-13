#include "block_core/search.hpp"
#include <algorithm>
#include <cmath>
#include "block_core/core.hpp"
namespace block_core {
void Search::reset() { direction_=attempts_=0; completed_=false; side_=turn_=dwell_=time_=0; }
void Search::set_side(double yaw_rate) { side_=yaw_rate?std::copysign(1.,yaw_rate):0.; }
void Search::reacquired() { direction_=0; turn_=dwell_=0; completed_=false; }
void Search::stop() { direction_=0; }
Search::Output Search::lost(double t,double dt,double& camera_deg,const SearchSettings& s) {
  Output out;
  if(completed_) return out;
  if(!direction_) {
    if(attempts_>=s.max_attempts) { completed_=true; return out; }
    direction_=1; ++attempts_; turn_=s.turn_sec; dwell_=0; time_=t;
  }
  const double elapsed=std::clamp(t-time_,0.,.1); time_=t;
  out.active=true;
  if(turn_>0 && side_) {
    turn_=std::max(0.,turn_-elapsed);
    // Same sign convention as tracking: yaw = gain * error_x, so a positive command turns the same way.
    auto w=wheel_speeds(0,side_*s.turn_speed_mps*2/s.track_width,s.track_width,s.max_wheel_speed,s.min_turn_speed);
    out.left=w.first; out.right=w.second; out.stationary=false;
  } else if(dwell_<s.dwell_sec) dwell_+=elapsed;
  if(!side_ || (turn_<=0 && dwell_>=s.dwell_sec)) camera_deg+=direction_*s.pitch_speed_deg*dt;
  const double limit=direction_>0?s.cam_max_deg:s.cam_min_deg;
  if((direction_>0 && camera_deg>=limit) || (direction_<0 && camera_deg<=limit)) {
    camera_deg=limit;
    if(direction_<0) { direction_=0; completed_=true; out.active=false; out.stationary=true; }
    else direction_=-1;
  }
  return out;
}
}
