#include "block_core/claw_opening.hpp"
#include <algorithm>
namespace block_core {
void ClawOpening::reset() { angles_.reset(); track_.reset(); opened_.reset(); }
void ClawOpening::target_changed(uint64_t track) { if(track_ && track!=*track_) reset(); }
void ClawOpening::follow(uint64_t new_track) { if(track_) track_=new_track; }
bool ClawOpening::capture(uint64_t track,bool trigger,std::optional<Angles> proposal,double lo,double hi) {
  if(angles_ || !trigger || !proposal) return false;
  angles_=Angles{std::clamp(proposal->first,lo,hi),std::clamp(proposal->second,lo,hi)};
  track_=track;
  return true;
}
std::optional<ClawOpening::Angles> ClawOpening::apply(uint64_t track,double t) {
  if(!angles_ || !track_ || *track_!=track) return std::nullopt;
  opened_=t; return angles_;
}
}
