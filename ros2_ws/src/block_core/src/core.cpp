#include "block_core/core.hpp"
#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
namespace block_core {
namespace {
constexpr double pi=3.14159265358979323846;
bool valid_box(const Box& b) {
  return b.observed && b.width>0 && b.height>0 && std::isfinite(b.x) &&
    std::isfinite(b.y) && std::isfinite(b.width) && std::isfinite(b.height) && std::isfinite(b.confidence);
}
bool better(const Box& a,const Box& b) {
  if(a.y+a.height!=b.y+b.height) return a.y+a.height>b.y+b.height;
  if(a.confidence!=b.confidence) return a.confidence>b.confidence;
  return a.track_id<b.track_id;
}
std::string checksum(const std::string& s) {
  unsigned v=0; for(unsigned char c:s) v^=c;
  std::ostringstream out; out<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<v;
  return out.str();
}
}
double iou(const Box& a,const Box& b) {
  const double w=std::min(a.x+a.width,b.x+b.width)-std::max(a.x,b.x);
  const double h=std::min(a.y+a.height,b.y+b.height)-std::max(a.y,b.y);
  if(w<=0 || h<=0) return 0.;
  const double intersection=w*h;
  return intersection/(a.width*a.height+b.width*b.height-intersection);
}
Lock::Lock(double threshold,double timeout):threshold_(threshold),timeout_(timeout) {}
void Lock::reset() {
  id_=0; has_id_=has_last_=loss_active_=false; select_time_.reset();
  elapsed_=0; seen_.clear(); blocked_.clear();
}
const Box* Lock::select(const std::vector<Box>& boxes,double t,bool searching,
                       double search_iou,const std::set<uint64_t>& known_ids,bool stationary) {
  const Box* found=nullptr;
  for(const auto& b:boxes) if(b.observed && has_id_ && b.track_id==id_) { found=&b; break; }
  // Snapshot before adding IDs from this frame. Never expand it during this loss.
  if(has_id_ && !found && !loss_active_) { blocked_=seen_; loss_active_=true; }
  if(known_ids.empty()) { for(const auto& b:boxes) seen_.insert(b.track_id); }
  else seen_.insert(known_ids.begin(),known_ids.end());
  if(std::isfinite(t) && select_time_ && !searching) elapsed_+=std::max(0.,t-*select_time_);
  select_time_=std::isfinite(t)?std::optional<double>(t):std::nullopt;
  if(!has_id_) {
    for(const auto& b:boxes) if(b.observed && (!found || better(b,*found))) found=&b;
    if(found) { id_=found->track_id; has_id_=true; }
  }
  const double threshold=searching && search_iou>=0 ? search_iou:threshold_;
  if(!found && std::isfinite(t) && has_last_ && threshold_>0 && stationary && elapsed_<=timeout_) {
    double best=-1.;
    for(const auto& b:boxes) {
      if(!b.observed || b.class_id!=last_.class_id || blocked_.count(b.track_id)) continue;
      const double overlap=iou(last_,b);
      if(overlap>best) { best=overlap; found=&b; }
    }
    if(best<threshold) found=nullptr;
    if(found) id_=found->track_id;
  }
  if(found && std::isfinite(t)) {
    last_=*found; has_last_=true; loss_active_=false; blocked_.clear(); elapsed_=0;
  }
  return found;
}
std::vector<double> CVFilter::step(double t,const std::vector<double>* m,double q,double r,double iv) {
  if(time_ && t<=*time_) return {};
  if(!time_) {
    if(!m) return {};
    state_={m->at(0),m->at(1),0,0};
    covariance_={}; for(int i=0;i<4;++i) covariance_[i][i]=iv;
  } else {
    const double dt=t-*time_;
    std::array<std::array<double,4>,4> f{}, predicted{}, fp{};
    for(int i=0;i<4;++i) f[i][i]=1;
    f[0][2]=f[1][3]=dt;
    state_[0]+=dt*state_[2]; state_[1]+=dt*state_[3];
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) for(int k=0;k<4;++k) fp[i][j]+=f[i][k]*covariance_[k][j];
    const double g[4][2]={{dt*dt/2,0},{0,dt*dt/2},{dt,0},{0,dt}};
    for(int i=0;i<4;++i) for(int j=0;j<4;++j) {
      for(int k=0;k<4;++k) predicted[i][j]+=fp[i][k]*f[j][k];
      for(int k=0;k<2;++k) predicted[i][j]+=q*g[i][k]*g[j][k];
    }
    covariance_=predicted;
    if(m) {
      const double a=predicted[0][0]+r,b=predicted[0][1],c=predicted[1][0],d=predicted[1][1]+r;
      const double determinant=a*d-b*c;
      double gain[4][2]{};
      const double residual[2]={m->at(0)-state_[0],m->at(1)-state_[1]};
      std::array<std::array<double,4>,4> correction{}, ap{}, updated{};
      for(int i=0;i<4;++i) {
        gain[i][0]=(predicted[i][0]*d-predicted[i][1]*c)/determinant;
        gain[i][1]=(-predicted[i][0]*b+predicted[i][1]*a)/determinant;
        state_[i]+=gain[i][0]*residual[0]+gain[i][1]*residual[1];
        correction[i][i]=1; correction[i][0]-=gain[i][0]; correction[i][1]-=gain[i][1];
      }
      // Joseph covariance update preserves symmetry and positive definiteness.
      for(int i=0;i<4;++i) for(int j=0;j<4;++j) for(int k=0;k<4;++k) ap[i][j]+=correction[i][k]*predicted[k][j];
      for(int i=0;i<4;++i) for(int j=0;j<4;++j) {
        for(int k=0;k<4;++k) updated[i][j]+=ap[i][k]*correction[j][k];
        for(int k=0;k<2;++k) updated[i][j]+=r*gain[i][k]*gain[j][k];
      }
      covariance_=updated;
    }
  }
  time_=t; if(m) last_seen_=t;
  return {state_[0],state_[1]};
}
void CVFilter::reset() { time_.reset(); last_seen_.reset(); state_={}; covariance_={}; }
double rate(double e,double d,double g,double l) { return std::abs(e)<=d?0.:std::clamp(g*e,-l,l); }
std::pair<double,double> gripper_angles(double left,double right,double width,double height,const GripperSettings& s) {
  if(!(width>0 && height>0 && std::isfinite(left) && std::isfinite(right) && 0<=left && left<right && right<=width)) return {NAN,NAN};
  const double d=s.mm_per_px*(right-left)+s.claw_margin_mm;
  const double ratio=std::clamp((s.link_offset-d/2)/s.link_length,-1.,1.);
  const double deg=std::clamp(s.claw_init_deg-s.link_ref_deg+std::acos(ratio)*180/pi,s.claw_init_deg,s.claw_max_deg);
  return {deg*pi/180,deg*pi/180};
}
std::pair<double,double> wheel_speeds(double forward,double yaw,double width,double limit,double min_turn) {
  validate_finite({forward,yaw,width,limit,min_turn});
  double half=yaw*width/2;
  if(yaw && std::abs(half)<min_turn) half=std::copysign(min_turn,half);
  return {std::clamp(forward-half,-limit,limit),std::clamp(forward+half,-limit,limit)};
}
void Sweep::advance(double dt) { elapsed=std::min(duration,elapsed+std::max(0.,dt)); }
double Sweep::yaw() const { return done()?0.:bias*std::exp(-elapsed/tau); }
std::vector<Box> candidates(const std::vector<Box>& boxes,int cls) {
  std::vector<Box> result;
  for(const auto& b:boxes) if(valid_box(b) && b.class_id==cls) result.push_back(b);
  std::stable_sort(result.begin(),result.end(),better); return result;
}
std::optional<Obstacle> obstacle_for(const Box& target,const std::vector<Box>& boxes) {
  std::optional<Obstacle> result;
  double best_width=0;
  for(const auto& b:boxes) {
    const double width=std::min(target.x+target.width,b.x+b.width)-std::max(target.x,b.x);
    if(!valid_box(b) || b.track_id==target.track_id || width<=0 || b.y+b.height<=target.y+target.height) continue;
    if(result) {
      const auto& old=result->box;
      if(b.y+b.height<old.y+old.height) continue;
      if(b.y+b.height==old.y+old.height && (width<best_width || (width==best_width &&
        (b.confidence<old.confidence || (b.confidence==old.confidence && b.track_id>=old.track_id))))) continue;
    }
    best_width=width;
    result=Obstacle{b,b.x+b.width/2<=target.x+target.width/2?-1:1,width/target.width,
      std::min(target.y+target.height,b.y+b.height)>std::max(target.y,b.y)};
  }
  return result;
}
void validate_finite(const std::vector<double>& values) { for(double x:values) if(!std::isfinite(x)) throw std::invalid_argument("nonfinite control value"); }
uint16_t crc16(const uint8_t* d,size_t n) {
  uint16_t c=0xffff;
  for(size_t i=0;i<n;++i) { c^=d[i]; for(int j=0;j<8;++j) c=(c&1)?(c>>1)^0xa001:c>>1; }
  return c;
}
std::string encode_command(uint16_t seq,double lw,double rw,double lg,double rg,double cp) {
  validate_finite({lw,rw,lg,rg,cp});
  std::ostringstream out; out.imbue(std::locale::classic());
  out<<"CMD,"<<seq<<','<<std::fixed<<std::setprecision(3)<<lw<<','<<rw<<','<<std::setprecision(1)<<lg<<','<<rg<<','<<cp;
  const auto body=out.str(); return '$'+body+'*'+checksum(body);
}
std::optional<std::pair<uint16_t,double>> decode_feedback(const std::string& input) {
  const auto begin=input.find_first_not_of(" \r\n\t"),end=input.find_last_not_of(" \r\n\t");
  if(begin==std::string::npos) return {};
  const auto line=input.substr(begin,end-begin+1); const auto star=line.find('*');
  if(line.front()!='$' || star==std::string::npos) return {};
  const auto body=line.substr(1,star-1); auto tail=line.substr(star+1);
  for(auto& c:tail) c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if(tail!=checksum(body) || body.rfind("FB,",0)!=0) return {};
  const auto comma=body.find(',',3);
  if(comma==std::string::npos || body.find(',',comma+1)!=std::string::npos) return {};
  try {
    const auto a=body.substr(3,comma-3),b=body.substr(comma+1); size_t an=0,bn=0;
    const auto seq=std::stol(a,&an); const auto pitch=std::stod(b,&bn);
    if(an!=a.size() || bn!=b.size() || seq<0 || seq>=65536 || !std::isfinite(pitch)) return {};
    return std::pair<uint16_t,double>{static_cast<uint16_t>(seq),pitch};
  } catch(const std::exception&) { return {}; }
}
}
