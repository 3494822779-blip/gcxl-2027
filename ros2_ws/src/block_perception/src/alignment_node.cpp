#include <block_perception/parameter_validation.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <block_interfaces/msg/target.hpp>
#include <block_interfaces/msg/alignment_command.hpp>
#include <block_core/core.hpp>
using block_interfaces::msg::Target;
using block_interfaces::msg::AlignmentCommand;
class Alignment final : public rclcpp::Node {
 public:
  Alignment():Node("alignment") {
    declare_parameter("camera_servo",false); declare_parameter("pitch_joint",std::string("camera_pitch"));
    for(const auto& p:std::vector<std::pair<std::string,double>>{
      {"deadzone_x",.04},{"deadzone_y",.1},{"bottom_target",.5},{"forward_speed",.5},
      {"forward_max_error_x",.3},{"grasp_error_x",.15},{"yaw_near_weight",.2},{"yaw_center_weight",.3},
      {"yaw_full_error_x",.5},{"yaw_gain",-1},{"pitch_gain",1.5},{"max_yaw_rate",2},
      {"max_pitch_rate",.2},{"grasp_pitch",40*M_PI/180},{"pitch_timeout",.25},{"input_timeout",.25},
      {"hold_seconds",.3},{"reverse_speed",.1},{"grasp_pitch_tol",1.7*M_PI/180},
      {"approach_min_speed",.1},{"approach_slow_pitch_deg",15},{"too_close_y",.9}}) declare_parameter(p.first,p.second);
    parameter_guard_=parameter_guard(*this);
    pub_=create_publisher<AlignmentCommand>("alignment/command",10);
    sub_=create_subscription<Target>("target/filtered",10,[this](Target::ConstSharedPtr m){
      if(target_ && m->locked && rclcpp::Time(m->header.stamp)<=rclcpp::Time(target_->header.stamp)) return;
      target_=*m;
    });
    joints_=create_subscription<sensor_msgs::msg::JointState>("camera/joint_states",rclcpp::SensorDataQoS(),[this](sensor_msgs::msg::JointState::ConstSharedPtr m){
      const auto name=get_parameter("pitch_joint").as_string();
      for(size_t i=0;i<m->name.size() && i<m->position.size();++i) {
        const double t=rclcpp::Time(m->header.stamp).seconds();
        if(m->name[i]==name && std::isfinite(m->position[i]) && (!pitch_time_ || t>*pitch_time_)) {
          pitch_=m->position[i]; pitch_time_=t; break;
        }
      }
    });
    timer_=create_wall_timer(std::chrono::milliseconds(50),[this]{tick();});
  }
 private:
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard_;
  double p(const char* name) const { return get_parameter(name).as_double(); }
  void tick() {
    const double t=now().seconds();
    if(last_now_ && t<*last_now_) { target_.reset(); pitch_time_.reset(); }
    last_now_=t; AlignmentCommand out; out.header.stamp=now(); out.valid_for_sec=p("input_timeout"); out.reason="no_target";
    if(target_) {
      const auto& m=*target_;
      out.first_grasp_id=m.first_grasp_id; out.clearance=m.clearance; out.track_id=m.track_id; out.class_id=m.class_id;
      const double age=t-rclcpp::Time(m.header.stamp).seconds();
      if(age>=0 && age<=p("input_timeout") && m.valid && m.locked && m.image_width>0 && m.image_height>0 && std::isfinite(m.x) && std::isfinite(m.y)) {
        out.header=m.header; out.valid=true; out.predicted=!m.observed;
        out.obstacle_id=m.obstacle_id; out.sweep_side=m.sweep_side; out.obstacle_overlap=m.obstacle_overlap; out.obstacle_near=m.obstacle_near;
        out.error_x=(m.x-m.image_width/2.)/(m.image_width/2.);
        out.error_y=(m.y-p("bottom_target")*m.image_height)/(m.image_height/2.);
        const double near=std::clamp(m.y/(p("bottom_target")*m.image_height),0.,1.);
        const double dead=p("deadzone_x")+std::max(0.,p("grasp_error_x")-p("deadzone_x"))*near;
        const double offset=std::max(p("yaw_center_weight"),std::min(1.,std::abs(out.error_x)/p("yaw_full_error_x")));
        out.yaw_rate=block_core::rate(out.error_x,dead,p("yaw_gain")*std::max(p("yaw_near_weight"),1-near)*offset,p("max_yaw_rate"));
        out.aligned=std::abs(out.error_x)<=dead && std::abs(out.error_y)<=p("deadzone_y");
        out.open_allowed=std::abs(out.error_y)<=p("deadzone_y") && m.observed;
        const bool centred=std::abs(out.error_x)<=p("forward_max_error_x");
        bool ready=false; out.reason="aligning";
        if(get_parameter("camera_servo").as_bool()) {
          out.pitch_rate=block_core::rate(out.error_y,p("deadzone_y"),p("pitch_gain"),p("max_pitch_rate"));
          const bool fresh=pitch_time_ && t-*pitch_time_>=0 && t-*pitch_time_<=p("pitch_timeout");
          const double threshold=p("grasp_pitch")-p("grasp_pitch_tol");
          ready=out.aligned && m.observed && fresh && pitch_>=threshold;
          if(fresh && pitch_<threshold && centred) {
            const double weight=std::clamp((threshold-pitch_)*180/M_PI/p("approach_slow_pitch_deg"),0.,1.);
            out.forward_speed=std::min(p("forward_speed"),p("approach_min_speed")+(p("forward_speed")-p("approach_min_speed"))*weight);
            out.reason="approaching";
            if(m.y/m.image_height>=p("too_close_y")) { out.forward_speed=out.yaw_rate=0; out.reason="too_close_waiting_pitch"; }
          } else if(out.aligned) out.reason="aligned_waiting_pitch_or_hold";
        } else {
          ready=out.aligned && m.observed;
          if(out.error_y<-p("deadzone_y") && centred) { out.forward_speed=p("forward_speed"); out.reason="approaching"; }
          else if(out.error_y>p("deadzone_y")) { out.forward_speed=centred && m.observed?-p("reverse_speed"):0.; out.reason="too_close"; }
          else if(out.aligned) out.reason="aligned_waiting_hold";
        }
        out.grasp_request=ready;
        if(ready) { out.yaw_rate=out.pitch_rate=out.forward_speed=0; out.reason="grasp_ready"; }
        if(m.first_grasp_id && !m.observed) { out.yaw_rate=out.pitch_rate=out.forward_speed=0; out.reason="waiting_for_locked_target"; }
      } else out.reason="stale_or_invalid_target";
    }
    pub_->publish(out);
  }
  std::optional<Target> target_; std::optional<double> pitch_time_,last_now_; double pitch_{};
  rclcpp::Publisher<AlignmentCommand>::SharedPtr pub_; rclcpp::Subscription<Target>::SharedPtr sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joints_; rclcpp::TimerBase::SharedPtr timer_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<Alignment>()); rclcpp::shutdown(); return 0; }
