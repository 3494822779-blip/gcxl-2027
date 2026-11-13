#include <block_perception/parameter_validation.hpp>
#include <rclcpp/rclcpp.hpp>
#include <block_interfaces/msg/target.hpp>
#include <block_interfaces/msg/frame_metadata.hpp>
#include <block_interfaces/msg/gripper_angles.hpp>
#include <block_core/core.hpp>
#include <deque>
using namespace block_interfaces::msg;
class Gripper final : public rclcpp::Node {
 public:
  Gripper():Node("gripper") {
    declare_parameter("input_timeout",.25); declare_parameter("claw_init_deg",0.);
    declare_parameter("claw_max_deg",90.); declare_parameter("claw_margin_mm",5.);
    declare_parameter("mm_per_px",.5); declare_parameter("link_ref_deg",60.);
    declare_parameter("link_offset",44.); declare_parameter("link_length",91.);
    parameter_guard_=parameter_guard(*this);
    pub_=create_publisher<GripperAngles>("gripper/angles",10);
    frames_=create_subscription<FrameMetadata>("tracks_meta",rclcpp::SensorDataQoS(),[this](FrameMetadata::ConstSharedPtr m){
      for(auto it=cache_.begin();it!=cache_.end();++it) if(it->header==m->header) { cache_.erase(it); break; }
      cache_.push_back(*m); while(cache_.size()>30) cache_.pop_front(); match();
    });
    targets_=create_subscription<Target>("target/raw",10,[this](Target::ConstSharedPtr m){
      if(current_ && rclcpp::Time(m->header.stamp)<=rclcpp::Time(current_->header.stamp)) return;
      current_=*m;
      pub_->publish(invalid(m->locked && m->valid && m->observed?"waiting_for_matching_frame":"target_not_observed")); match();
    });
    timer_=create_wall_timer(std::chrono::milliseconds(50),[this]{
      const auto t=now().nanoseconds();
      if(last_time_ && t<*last_time_) { cache_.clear(); current_.reset(); }
      last_time_=t;
      if(!current_) pub_->publish(invalid("no_target"));
      else { const double age=(now()-rclcpp::Time(current_->header.stamp)).seconds();
        if(age<0 || age>get_parameter("input_timeout").as_double()) pub_->publish(invalid("stale_target")); }
    });
  }
 private:
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard_;
  GripperAngles invalid(const std::string& reason) {
    GripperAngles out; out.left_angle_rad=out.right_angle_rad=NAN;
    out.valid_for_sec=get_parameter("input_timeout").as_double(); out.reason=reason;
    if(current_) { out.header=current_->header; out.track_id=current_->track_id;
      out.image_width=current_->image_width; out.image_height=current_->image_height; }
    else out.header.stamp=now();
    return out;
  }
  void match() {
    if(!current_ || !current_->locked || !current_->valid || !current_->observed) return;
    auto it=std::find_if(cache_.begin(),cache_.end(),[this](const auto& f){return f.header==current_->header;});
    if(it==cache_.end()) return;
    const auto frame=*it; cache_.erase(it); auto out=invalid("invalid_bounds");
    const double age=(now()-rclcpp::Time(current_->header.stamp)).seconds();
    if(age<0 || age>get_parameter("input_timeout").as_double()) out.reason="stale_target";
    else if(frame.width==current_->image_width && frame.height==current_->image_height) {
      const Box* box=nullptr; size_t matches=0;
      for(const auto& b:frame.boxes) if(b.observed && b.track_id==current_->track_id) { box=&b; ++matches; }
      if(matches==1 && frame.height>0 && std::isfinite(box->x) && std::isfinite(box->width) && box->x>=0 && box->width>0 && box->x+box->width<=frame.width) {
        out.bounds_valid=true; out.left_edge_px=box->x; out.right_edge_px=box->x+box->width;
        block_core::GripperSettings s;
        s.mm_per_px=get_parameter("mm_per_px").as_double(); s.claw_margin_mm=get_parameter("claw_margin_mm").as_double();
        s.link_ref_deg=get_parameter("link_ref_deg").as_double(); s.link_offset=get_parameter("link_offset").as_double();
        s.link_length=get_parameter("link_length").as_double(); s.claw_init_deg=get_parameter("claw_init_deg").as_double();
        s.claw_max_deg=get_parameter("claw_max_deg").as_double();
        const auto angles=block_core::gripper_angles(out.left_edge_px,out.right_edge_px,frame.width,frame.height,s);
        if(std::isfinite(angles.first) && std::isfinite(angles.second)) {
          out.left_angle_rad=angles.first; out.right_angle_rad=angles.second; out.angles_ready=true; out.reason="ready";
        } else out.reason="invalid_calculation_result";
      }
    }
    pub_->publish(out);
  }
  std::deque<FrameMetadata> cache_; std::optional<Target> current_; std::optional<int64_t> last_time_;
  rclcpp::Publisher<GripperAngles>::SharedPtr pub_; rclcpp::Subscription<FrameMetadata>::SharedPtr frames_;
  rclcpp::Subscription<Target>::SharedPtr targets_; rclcpp::TimerBase::SharedPtr timer_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<Gripper>()); rclcpp::shutdown(); return 0; }
