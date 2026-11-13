#include <block_actuator/parameter_validation.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <block_interfaces/msg/alignment_command.hpp>
#include <block_interfaces/msg/gripper_angles.hpp>
#include <block_interfaces/msg/target_task.hpp>
#include <block_interfaces/msg/target_search.hpp>
#include <block_core/search.hpp>
#include <block_core/sweep_control.hpp>
#include <block_core/claw_opening.hpp>
#include <block_interfaces/action/robot_task.hpp>
#include <block_core/core.hpp>
#include <block_core/task_memory.hpp>
#include <block_core/debug_log.hpp>
#include <map>
#include <set>
#include <optional>
#include <signal.h>
#include <unistd.h>
using namespace block_interfaces::msg;
using Action=block_interfaces::action::RobotTask;
using GoalHandle=rclcpp_action::ServerGoalHandle<Action>;
namespace {
double steady() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
volatile sig_atomic_t stopping=0;
void signal_stop(int) { stopping=1; }
bool target_operation(const std::string& op) { return op=="target_grasp" || op=="approach_grasp" || op=="first_grasp"; }
}
class Actuator final : public rclcpp::Node {
 public:
  Actuator():Node("actuator") {
    declare_parameter("camera_servo",false);
    declare_parameter("camera_fixed_deg",0.0);
    declare_parameter("track_width",0.2);
    declare_parameter("wheel_speed_scale",1000.0);
    declare_parameter("max_wheel_speed",0.5);
    declare_parameter("min_turn_speed",0.1);
    declare_parameter("cam_min_deg",0.0);
    declare_parameter("cam_max_deg",90.0);
    declare_parameter("search_pitch_speed_deg",20.0);
    declare_parameter("search_dwell_sec",0.15);
    declare_parameter("search_turn_sec",0.35);
    declare_parameter("search_turn_speed_mps",0.17);
    declare_parameter("search_max_attempts",3);
    // Keep the actuator's grasp target aligned with the perception target and
    // bringup configuration (45 degrees).  A mismatch here makes the camera
    // stop at one angle while the readiness gate waits for another one.
    declare_parameter("grasp_pitch_deg",45.0);
    declare_parameter("grasp_pitch_tol",1.7 * M_PI / 180.);
    declare_parameter("claw_open_pitch_deg",20.0);
    declare_parameter("claw_init_deg",0.0);
    declare_parameter("claw_max_deg",90.0);
    declare_parameter("claw_min_deg",-30.0);
    declare_parameter("sweep_claw_deg",-30.0);
    declare_parameter("sweep_overlap",0.5);
    declare_parameter("sweep_weight",0.2);
    declare_parameter("sweep_near_multiplier",2.0);
    declare_parameter("sweep_max_yaw_rate",1.0);
    declare_parameter("sweep_tau_sec",0.33);
    declare_parameter("sweep_duration_sec",1.0);
    declare_parameter("max_yaw_rate",2.0);
    declare_parameter("autonomous_target_class",-1);
    declare_parameter("left_claw_close_deg",0.0);
    declare_parameter("right_claw_close_deg",5.0);
    declare_parameter("pitch_home",0.0);
    declare_parameter("pitch_home_tol",0.03);
    declare_parameter("discard_enabled",false);
    declare_parameter("discard_distance_m",0.2);
    declare_parameter("discard_speed_mps",0.1);
    declare_parameter("discard_turn_speed_mps",0.17);
    declare_parameter("claw_settle_sec",0.5);
    declare_parameter("claw_deadband_deg",0.0);
    declare_parameter("pitch_timeout",0.25);
    declare_parameter("pitch_joint",std::string("camera_pitch"));
    declare_parameter("input_timeout",0.25);
    declare_parameter("serial_health_timeout",0.25);
    declare_parameter("pitch_feedback_margin_deg",2.0);
    declare_parameter("feedback_reconnect_sec",1.0);
    declare_parameter("max_sweep_attempts",2);
    declare_parameter("max_sweeps",8);
    declare_parameter("max_pitch_rate",0.6);
    declare_parameter("max_tracking_motion_sec",10.0);
    rcl_interfaces::msg::ParameterDescriptor fixed; fixed.read_only=true;
    declare_parameter("require_serial_health",false,fixed); declare_parameter("task_mode",std::string("normal"),fixed); declare_parameter("autonomous",false,fixed);
    parameter_guard_=parameter_guard(*this);
    left_=right_=p("claw_init_deg"); camera_=std::clamp(p("camera_fixed_deg"),p("cam_min_deg"),p("cam_max_deg")); safe_={left_,right_,camera_};
    tx_=create_publisher<std_msgs::msg::String>("serial_tx",10);
    safety_pub_=create_publisher<std_msgs::msg::String>("robot/safety",rclcpp::QoS(1).transient_local()); publish_safety("ready");
    task_pub_=create_publisher<TargetTask>("task/target_grasp",rclcpp::QoS(1).transient_local());
    reset_pub_=create_publisher<std_msgs::msg::Bool>("target/reset",10);
    search_pub_=create_publisher<TargetSearch>("task/target_search",10);
    finished_pub_=create_publisher<AlignmentCommand>("task/grasp_finished",10);
    joints_pub_=create_publisher<sensor_msgs::msg::JointState>("camera/joint_states",rclcpp::SensorDataQoS());
    commands_=create_subscription<AlignmentCommand>("alignment/command",10,[this](AlignmentCommand::ConstSharedPtr m){command(*m);});
    angles_=create_subscription<GripperAngles>("gripper/angles",10,[this](GripperAngles::ConstSharedPtr m){
      if(stamp(*m)<=resume_after_) return;
      if(!m->angles_ready && m->reason=="waiting_for_matching_frame" && grip_ && fresh(*grip_,grip_received_) && grip_->angles_ready && grip_->track_id==m->track_id) return;
      if(!grip_ || stamp(*m)>stamp(*grip_)) grip_received_=steady();
      grip_=*m;
    });
    health_=create_subscription<std_msgs::msg::Bool>("serial/healthy",rclcpp::QoS(1).transient_local(),[this](std_msgs::msg::Bool::ConstSharedPtr m){
      healthy_=m->data; health_time_=steady(); if(!m->data && required_health() && goal_) trip("serial_link_lost");
    });
    rx_=create_subscription<std_msgs::msg::String>("serial_rx",10,[this](std_msgs::msg::String::ConstSharedPtr m){feedback(m->data);});
    joints_=create_subscription<sensor_msgs::msg::JointState>("camera/joint_states",rclcpp::SensorDataQoS(),[this](sensor_msgs::msg::JointState::ConstSharedPtr m){joint(*m);});
    emergency_=create_service<std_srvs::srv::Trigger>("robot/emergency_stop",[this](std::shared_ptr<std_srvs::srv::Trigger::Request>,std::shared_ptr<std_srvs::srv::Trigger::Response> response){trip("emergency_stop"); response->success=true; response->message="stopped; explicit reset required";});
    reset_=create_service<std_srvs::srv::Trigger>("robot/reset_safety",[this](std::shared_ptr<std_srvs::srv::Trigger::Request>,std::shared_ptr<std_srvs::srv::Trigger::Response> response){
      response->success=!goal_ && link_ready();
      if(response->success) { safety_.clear(); hold(); publish_safety("ready"); response->message="ready; submit a new task to move"; }
      else response->message="task still ending or serial link unavailable";
    });
    server_=rclcpp_action::create_server<Action>(this,"actuator/execute",
      [this](const rclcpp_action::GoalUUID& id,std::shared_ptr<const Action::Goal> g){
        if(!valid_goal(*g)) return rclcpp_action::GoalResponse::REJECT;
        autonomous_pending_=false;
        if(g->operation=="stop") { ++generation_; admissions_.clear(); reserved_=false; finish(false,"stopped by stop action"); stop(); return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE; }
        if(reserved_ || goal_ || !safety_.empty() || !link_ready()) return rclcpp_action::GoalResponse::REJECT;
        reserved_=true; admissions_[id]=generation_; return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
      },[](auto){return rclcpp_action::CancelResponse::ACCEPT;},[this](auto h){start(h);});
    memory_pub_=create_publisher<std_msgs::msg::String>("robot/memory",rclcpp::QoS(1).transient_local());
    memory_service_=create_service<std_srvs::srv::Trigger>("robot/memory/get",[this](std::shared_ptr<std_srvs::srv::Trigger::Request>,std::shared_ptr<std_srvs::srv::Trigger::Response> response){response->success=true; response->message=block_core::TaskMemory::encode(memory_.snapshot());});
    publish_memory();
    auto_client_=rclcpp_action::create_client<Action>(this,"robot/execute");
    autonomous_pending_=get_parameter("autonomous").as_bool();
    timer_=create_wall_timer(std::chrono::milliseconds(50),[this]{
      if(autonomous_pending_ && link_ready() && auto_client_->action_server_is_ready()) {
        autonomous_pending_=false;
        const int cls=static_cast<int>(p("autonomous_target_class"));
        if(cls>=0) { Action::Goal g; g.operation="target_grasp"; g.has_target_class=true; g.target_class_id=cls; auto_client_->async_send_goal(g); }
        else RCLCPP_ERROR(get_logger(),"autonomous requires autonomous_target_class; staying stopped");
      }
      tick();
    });
  }
  void publish_memory() { if(memory_revision_==memory_.revision()) return; std_msgs::msg::String m; m.data=block_core::TaskMemory::encode(memory_.context()); memory_pub_->publish(m); memory_revision_=memory_.revision(); }
  void stop() {
    left_=safe_[0]; right_=safe_[1]; camera_=safe_[2]; send(0,0);
  }
 private:
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard_;
  double p(const char* name) const { const auto value=get_parameter(name); return value.get_type()==rclcpp::ParameterType::PARAMETER_INTEGER?value.as_int():value.as_double(); }
  bool servo() const { return get_parameter("camera_servo").as_bool(); }
  bool required_health() const { return get_parameter("require_serial_health").as_bool(); }
  template<class M> double stamp(const M& m) const { return rclcpp::Time(m.header.stamp).seconds(); }
  template<class M> bool fresh(const M& m,double received) const {
    const double ttl=std::min(double(m.valid_for_sec),p("input_timeout"));
    const double age=now().seconds()-stamp(m),elapsed=steady()-received;
    return std::isfinite(ttl) && ttl>0 && age>=0 && age<=ttl && elapsed>=0 && elapsed<=ttl;
  }
  bool pitch_fresh() const { return pitch_time_ && now().seconds()-*pitch_time_>=0 && now().seconds()-*pitch_time_<=p("pitch_timeout") && steady()-pitch_received_<=p("pitch_timeout"); }
  bool link_ready() const { return !required_health() || (healthy_ && health_time_ && steady()-*health_time_<=p("serial_health_timeout")); }
  void publish_safety(const std::string& text) { std_msgs::msg::String m; m.data=text; safety_pub_->publish(m); }
  void trip(const std::string& reason) {
    autonomous_pending_=false; ++generation_; admissions_.clear(); reserved_=false;
    if(safety_.empty()) { safety_=reason; publish_safety(reason); }
    finish(false,reason); hold(); stop();
  }
  void hold() { state_="hold"; cmd_.reset(); grip_.reset(); cmd_stamp_.reset(); search_.stop(); resume_after_=now().seconds(); }
  bool valid_goal(const Action::Goal& g) const {
    static const std::set<std::string> ops{"align","grasp","target_grasp","approach_grasp","first_grasp","move","turn","claw","camera","stop"};
    if(!ops.count(g.operation) || g.discard_after || !std::isfinite(g.timeout_sec) || !std::isfinite(g.distance_m) || !std::isfinite(g.angle_deg) || !std::isfinite(g.speed_mps) || g.timeout_sec<0 || g.speed_mps<0) return false;
    if(target_operation(g.operation) && (!g.has_target_class || g.target_class_id<0)) return false;
    if(g.operation=="claw" && (g.angle_deg<p("claw_min_deg") || g.angle_deg>p("claw_max_deg"))) return false;
    if(g.operation=="camera" && (!servo() || g.angle_deg<p("cam_min_deg") || g.angle_deg>p("cam_max_deg"))) return false;
    return true;
  }
  void start(const std::shared_ptr<GoalHandle>& handle) {
    const auto g=handle->get_goal();
    if(g->operation=="stop") { stop(); auto r=std::make_shared<Action::Result>(); r->success=true; r->message="wheels stopped; servo targets held"; handle->succeed(r); return; }
    const auto admission=admissions_.find(handle->get_goal_id());
    const bool admitted=admission!=admissions_.end() && admission->second==generation_;
    if(admission!=admissions_.end()) admissions_.erase(admission);
    reserved_=!admissions_.empty();
    if(!admitted || !safety_.empty() || !link_ready()) { auto r=std::make_shared<Action::Result>(); r->message="interrupted before execution"; handle->abort(r); return; }
    goal_=handle; started_=steady(); timeout_=g->timeout_sec>0?g->timeout_sec:target_operation(g->operation)?180:60;
    hold(); action_.reset(); close_time_.reset(); sweep_ctl_.reset(); claw_open_.reset(); formal_track_.reset();
    sweep_time_.reset(); search_.reset();
    motion_budget_=0; motion_active_=false; task_=0;
    if(!target_operation(g->operation)) memory_.begin_task(g->operation);
    if(target_operation(g->operation)) {
      task_=std::max<uint64_t>(last_task_+1,std::max<int64_t>(1,now().nanoseconds())); last_task_=task_;
      memory_.begin_task(g->operation,task_); memory_.target_class(g->target_class_id);
      TargetTask m; m.task_id=task_; m.target_class_id=g->target_class_id; m.active=true; task_pub_->publish(m); state_="track";
    } else if(g->operation=="align") { std_msgs::msg::Bool m; m.data=true; reset_pub_->publish(m); state_="track"; }
    else if(g->operation=="grasp") begin_grasp();
    else if(g->operation=="move" || g->operation=="turn") {
      speed_=g->speed_mps>0?g->speed_mps:p(g->operation=="move"?"discard_speed_mps":"discard_turn_speed_mps");
      if(g->operation=="turn") speed_=std::max(speed_,p("min_turn_speed"));
      speed_=std::min(speed_,p("max_wheel_speed"));
      duration_=g->operation=="move"?std::abs(g->distance_m)/speed_:std::abs(g->angle_deg)*M_PI/180*p("track_width")/(2*speed_);
    } else if(g->operation=="claw") { if(left_!=g->angle_deg || right_!=g->angle_deg) memory_.uncertain_hold("manual_claw_command"); left_=right_=g->angle_deg; }
    else if(g->operation=="camera") { camera_=g->angle_deg; camera_initialized_=true; }
  }
  void finish(bool success,const std::string& message,bool canceled=false) {
    if(!goal_) return;
    auto r=std::make_shared<Action::Result>(); r->success=success; r->message=message;
    r->track_id=action_?action_->track_id:cmd_?cmd_->track_id:0;
    memory_.finish_task(success?"completed":canceled?"canceled":message.rfind("timeout",0)==0?"timeout":"aborted",state_,message); publish_memory();
    if(task_) { TargetTask m; m.task_id=task_; m.active=false; task_pub_->publish(m); }
    if(canceled && goal_->is_canceling()) goal_->canceled(r); else if(success) goal_->succeed(r); else goal_->abort(r);
    goal_.reset(); reserved_=false; hold();
  }
  bool allows_grasp() const { return goal_ && target_operation(goal_->get_goal()->operation); }
  bool allows(const AlignmentCommand& m) const {
    if(!goal_) return false;
    return (goal_->get_goal()->operation=="align" && !m.first_grasp_id) ||
      (allows_grasp() && m.first_grasp_id==task_ && m.class_id==goal_->get_goal()->target_class_id && !m.clearance);
  }
  void command(const AlignmentCommand& m) {
    if(!allows(m) || stamp(m)<=resume_after_ || !safety_.empty()) return;
    for(double value:{m.yaw_rate,m.pitch_rate,m.forward_speed,m.error_x,m.error_y,m.valid_for_sec,m.obstacle_overlap}) if(!std::isfinite(value)) { trip("invalid_alignment_command"); return; }
    if(m.valid_for_sec<=0 || m.obstacle_overlap<0 || m.obstacle_overlap>1 || std::abs(m.sweep_side)>1 || std::abs(m.forward_speed)>p("max_wheel_speed") || std::abs(m.yaw_rate)>p("max_yaw_rate") || std::abs(m.pitch_rate)>p("max_pitch_rate")) { trip("invalid_alignment_command"); return; }
    if(cmd_stamp_ && stamp(m)<=*cmd_stamp_) return;
    cmd_stamp_=stamp(m); cmd_received_=steady(); cmd_=m;
    if(state_!="track" || !m.valid || m.predicted || !fresh(m,cmd_received_) || !allows_grasp()) return;
    search_.set_side(m.yaw_rate);
    if(formal_track_ && m.track_id!=*formal_track_) claw_open_.follow(m.track_id);
    formal_track_=m.track_id;
    if(sweep_ctl_.ready(steady(),stamp(m),sweep_settings()) && m.sweep_side) {
      auto started=sweep_ctl_.begin(m.sweep_side,m.obstacle_overlap,m.obstacle_near,m.obstacle_id,steady(),{left_,right_},sweep_settings());
      if(!started) { trip("sweep_no_progress"); return; }
      memory_.begin_sweep(m.track_id,m.class_id,m.obstacle_id,m.sweep_side);
      if(started->side<0) left_=started->claw_deg; else right_=started->claw_deg;
      claw_open_.invalidate_timer();
    }
    search_.stop();
    if(blocker().empty()) begin_grasp(m);
  }
  block_core::SweepSettings sweep_settings() const {
    block_core::SweepSettings s;
    s.claw_deg=p("sweep_claw_deg"); s.overlap=p("sweep_overlap"); s.weight=p("sweep_weight");
    s.near_multiplier=p("sweep_near_multiplier"); s.max_yaw_rate=p("sweep_max_yaw_rate");
    s.tau_sec=p("sweep_tau_sec"); s.duration_sec=p("sweep_duration_sec"); s.settle_sec=p("claw_settle_sec");
    s.max_attempts=static_cast<int>(p("max_sweep_attempts")); s.max_sweeps=static_cast<int>(p("max_sweeps"));
    return s;
  }
  block_core::SearchSettings search_settings() const {
    block_core::SearchSettings s;
    s.turn_sec=p("search_turn_sec"); s.dwell_sec=p("search_dwell_sec"); s.turn_speed_mps=p("search_turn_speed_mps");
    s.pitch_speed_deg=p("search_pitch_speed_deg"); s.cam_min_deg=p("cam_min_deg"); s.cam_max_deg=p("cam_max_deg");
    s.track_width=p("track_width"); s.max_wheel_speed=p("max_wheel_speed"); s.min_turn_speed=p("min_turn_speed");
    s.max_attempts=static_cast<int>(p("search_max_attempts"));
    return s;
  }
  std::string blocker() const {
    if(search_.completed() && (!cmd_ || !fresh(*cmd_,cmd_received_) || !cmd_->valid || cmd_->predicted)) return "search_exhausted";
    if(search_.direction()) return search_.direction()>0?"searching_down":"searching_up";
    if(!cmd_) return "waiting_for_target";
    if(!fresh(*cmd_,cmd_received_) || !cmd_->valid) return "stale_or_invalid_target";
    if(cmd_->predicted) return "waiting_for_locked_target";
    if(sweep_ctl_.active()) return "sweeping";
    if(sweep_ctl_.awaiting_observation(stamp(*cmd_))) return "waiting_for_post_sweep_observation";
    if(cmd_->sweep_side) return "waiting_for_sweep";
    if(!cmd_->grasp_request) return cmd_->reason.empty()?"aligning":cmd_->reason;
    if(claw_open_.needs_open(cmd_->track_id)) {
      if(servo() && (!pitch_fresh() || pitch_<p("claw_open_pitch_deg")*M_PI/180)) return "waiting_for_open_pitch";
      if(!grip_ || !fresh(*grip_,grip_received_) || !grip_->angles_ready || grip_->track_id!=cmd_->track_id) return "waiting_for_gripper_angles";
      return "waiting_for_opening";
    }
    if(!claw_open_.opened()) return "waiting_for_opening";
    return claw_open_.settling(steady(),p("claw_settle_sec"))?"opening":"";
  }
  void begin_grasp(std::optional<AlignmentCommand> target={}) { action_=target; state_="grasp"; close_time_.reset(); close_angles_={p("left_claw_close_deg"),p("right_claw_close_deg")}; memory_.begin_grasp(target?target->first_grasp_id:0,target?target->clearance:false,target?std::optional<uint64_t>(target->track_id):std::nullopt,target?std::optional<int>(target->class_id):std::nullopt,close_angles_.first,close_angles_.second); }
  bool close_ready() const {
    return !action_ || (cmd_ && fresh(*cmd_,cmd_received_) && cmd_->valid && !cmd_->predicted && cmd_->grasp_request && !cmd_->sweep_side &&
      cmd_->track_id==action_->track_id && cmd_->class_id==action_->class_id && cmd_->first_grasp_id==action_->first_grasp_id);
  }
  void joint(const sensor_msgs::msg::JointState& m) {
    for(size_t i=0;i<m.name.size() && i<m.position.size();++i) if(m.name[i]==get_parameter("pitch_joint").as_string()) {
      const double value=m.position[i],deg=value*180/M_PI,t=stamp(m),age=now().seconds()-t;
      if(std::isfinite(value) && deg>=p("cam_min_deg")-p("pitch_feedback_margin_deg") && deg<=p("cam_max_deg")+p("pitch_feedback_margin_deg") && age>=0 && age<=p("pitch_timeout") && (!pitch_time_ || t>*pitch_time_)) { pitch_=value; pitch_time_=t; pitch_received_=steady(); }
    }
  }
  void feedback(const std::string& chunk) {
    rx_buffer_+=chunk; if(rx_buffer_.size()>512) rx_buffer_.erase(0,rx_buffer_.size()-512);
    size_t end;
    while((end=rx_buffer_.find('\n'))!=std::string::npos) {
      const auto line=rx_buffer_.substr(0,end); rx_buffer_.erase(0,end+1); const auto fb=block_core::decode_feedback(line);
      if(!fb || fb->second<p("cam_min_deg")-p("pitch_feedback_margin_deg") || fb->second>p("cam_max_deg")+p("pitch_feedback_margin_deg")) continue;
      const double t=steady();
      if(!fb_seq_ || t-fb_time_>=p("feedback_reconnect_sec")) { fb_seq_=fb->first; fb_time_=t; continue; }
      const uint16_t advance=fb->first-*fb_seq_; if(!advance || advance>=32768) continue;
      fb_seq_=fb->first; fb_time_=t;
      sensor_msgs::msg::JointState m; m.header.stamp=now(); m.name={get_parameter("pitch_joint").as_string()}; m.position={fb->second*M_PI/180}; joint(m); joints_pub_->publish(m);
    }
  }
  void publish_search(bool active,bool stationary=true) {
    if(!task_) return; TargetSearch m; m.header.stamp=now(); m.task_id=task_; m.active=active; m.stationary=stationary; search_pub_->publish(m);
  }
  std::pair<double,double> compute() {
    double lw=0,rw=0; const double wall=now().seconds(),t=steady();
    const double dt=cam_time_?std::clamp(wall-*cam_time_,0.,.1):0.; cam_time_=wall;
    const bool pitch_ok=servo() && pitch_fresh(); if(!servo()) camera_=p("camera_fixed_deg");
    else if(!camera_initialized_) { camera_=pitch_ok?pitch_*180/M_PI:p("pitch_home")*180/M_PI; camera_initialized_=true; }
    if(!safety_.empty()) state_="hold";
    if(state_=="track") {
      const double step=sweep_time_?std::clamp(t-*sweep_time_,0.,.1):0; sweep_time_=t;
      bool valid=cmd_ && fresh(*cmd_,cmd_received_) && cmd_->valid && (!cmd_->first_grasp_id || !cmd_->predicted);
      if(allows_grasp() && formal_track_) valid=valid && cmd_->track_id==*formal_track_;
      const bool sweeping=sweep_ctl_.active();
      if(valid && (search_.direction() || search_.completed())) { search_.reacquired(); publish_search(false); }
      if(servo() && !pitch_ok) valid=false;
      if(!valid && servo() && pitch_ok && allows_grasp() && formal_track_ && !search_.completed()) {
        const auto out=search_.lost(t,dt,camera_,search_settings());
        lw=out.left; rw=out.right;
        publish_search(out.active,out.stationary);
      } else { search_.stop(); publish_search(false); }
      if(valid) {
        const auto& c=*cmd_; double bias=0; bool settling=false;
        if(sweeping) {
          const auto tk=sweep_ctl_.tick(t,step,wall,sweep_settings()); settling=tk.settling; bias=tk.bias;
          if(tk.finished) { memory_.complete_sweep(); left_=sweep_ctl_.restore().first; right_=sweep_ctl_.restore().second; claw_open_.restored(t); }
        }
        const bool recovering=sweep_ctl_.recovering(t,sweep_settings());
        double half=c.yaw_rate*p("track_width")/2;
        if(half && std::abs(half)<p("min_turn_speed")) half=std::copysign(p("min_turn_speed"),half);
        const double yaw=std::clamp(2*half/p("track_width")+bias,-p("max_yaw_rate"),p("max_yaw_rate"));
        auto wheels=block_core::wheel_speeds(c.grasp_request?0:c.forward_speed,yaw,p("track_width"),p("max_wheel_speed")); lw=wheels.first; rw=wheels.second;
        if(settling || recovering || (!sweeping && c.sweep_side)) lw=rw=0;
        if(servo()) camera_+=c.pitch_rate*180/M_PI*dt;
        claw_open_.target_changed(c.track_id);
        const bool trigger=servo()?pitch_ok && pitch_>=p("claw_open_pitch_deg")*M_PI/180:c.open_allowed;
        if(allows_grasp() && !c.predicted && !sweeping && !claw_open_.opened()) {
          std::optional<block_core::ClawOpening::Angles> proposal;
          if(grip_ && fresh(*grip_,grip_received_) && grip_->angles_ready && grip_->track_id==c.track_id && std::isfinite(grip_->left_angle_rad) && std::isfinite(grip_->right_angle_rad))
            proposal=block_core::ClawOpening::Angles{grip_->left_angle_rad*180/M_PI,grip_->right_angle_rad*180/M_PI};
          claw_open_.capture(c.track_id,trigger,proposal,p("claw_init_deg"),p("claw_max_deg"));
          if(auto angles=claw_open_.apply(c.track_id,t)) { memory_.uncertain_hold("visual_gripper_open_once"); left_=angles->first; right_=angles->second; }
        }
      }
      if(claw_open_.settling(t,p("claw_settle_sec"))) lw=rw=0;
    } else if(state_=="grasp") {
      bool reached=close_ready();
      if(servo()) { camera_=p("grasp_pitch_deg"); reached=reached && pitch_ok && pitch_>=p("grasp_pitch_deg")*M_PI/180-p("grasp_pitch_tol"); }
      if(reached || close_time_) {
        memory_.phase("claw_close"); left_=close_angles_.first; right_=close_angles_.second; if(!close_time_) close_time_=t;
        if(t-*close_time_>=p("claw_settle_sec")) {
          if(servo()) camera_=p("pitch_home")*180/M_PI; state_="done"; memory_.complete_grasp();
          if(action_ && action_->first_grasp_id) { auto result=*action_; result.header.stamp=now(); finished_pub_->publish(result); resume_after_=wall; cmd_.reset(); grip_.reset(); }
        }
      }
    } else if(state_=="done" && servo()) camera_=p("pitch_home")*180/M_PI;
    camera_=std::clamp(camera_,p("cam_min_deg"),p("cam_max_deg")); return {lw,rw};
  }
  void send(double lw,double rw) {
    std_msgs::msg::String m; m.data=block_core::encode_command(seq_++,lw*p("wheel_speed_scale"),rw*p("wheel_speed_scale"),left_,right_,camera_); tx_->publish(m); Json::Value fields; fields["state"]=state_; fields["frame"]=m.data; fields["pitch"]=pitch_; debug_.event("tx_command",now().seconds(),fields,.5);
  }
  void tick() {
    std::optional<std::pair<double,double>> direct;
    if(goal_) {
      const auto g=goal_->get_goal(); const double elapsed=steady()-started_;
      if(goal_->is_canceling()) { finish(false,"canceled; wheels stopped, servo targets held",true); stop(); }
      else if(elapsed>=timeout_) { finish(false,"timeout; wheels stopped, servo targets held"); stop(); }
      else if(!link_ready()) trip("serial_link_lost");
      else {
        auto feedback=std::make_shared<Action::Feedback>(); feedback->elapsed_sec=elapsed;
        feedback->phase=state_=="track"?blocker():state_=="grasp"?(close_time_?"claw_close":"grasp_pitch"):g->operation;
        feedback->track_id=cmd_?cmd_->track_id:action_?action_->track_id:0; feedback->clearance=sweep_ctl_.active() && state_=="track"; goal_->publish_feedback(feedback); memory_.phase(feedback->phase);
        Json::Value fields;fields["reason"]=feedback->phase;fields["track_id"]=Json::UInt64(feedback->track_id);debug_.event("grasp_wait",now().seconds(),fields,.5);
        bool done=false;
        if(g->operation=="move" || g->operation=="turn") {
          done=elapsed>=duration_;
          if(!done) { const double speed=std::copysign(speed_,g->operation=="move"?g->distance_m:g->angle_deg); direct=std::pair<double,double>{g->operation=="move"?speed:-speed,speed}; }
        } else if(g->operation=="claw") done=elapsed>=p("claw_settle_sec");
        else if(g->operation=="camera") done=pitch_fresh() && std::abs(pitch_-g->angle_deg*M_PI/180)<=p("grasp_pitch_tol");
        else if(g->operation=="align") done=cmd_ && fresh(*cmd_,cmd_received_) && cmd_->valid && !cmd_->predicted && cmd_->aligned;
        else done=state_=="done";
        if(done) finish(true,target_operation(g->operation) || g->operation=="grasp"?"completed; physical grasp unconfirmed":"completed (timed motion is not odometry confirmation)");
      }
    }
    try {
      auto wheels=compute(); if(direct && safety_.empty()) wheels=*direct;
      const double t=steady();
      if(motion_active_ && motion_time_) motion_budget_+=std::clamp(t-*motion_time_,0.,p("input_timeout"));
      motion_time_=t; motion_active_=state_=="track" && (wheels.first!=0 || wheels.second!=0);
      if(motion_active_ && motion_budget_>=p("max_tracking_motion_sec")) throw std::runtime_error("tracking_motion_budget_exceeded");
      block_core::validate_finite({wheels.first,wheels.second,left_,right_,camera_});
      if(std::max(std::abs(wheels.first),std::abs(wheels.second))>p("max_wheel_speed")+1e-9 || left_<p("claw_min_deg") || left_>p("claw_max_deg") || right_<p("claw_min_deg") || right_>p("claw_max_deg") || camera_<p("cam_min_deg") || camera_>p("cam_max_deg")) throw std::runtime_error("invalid actuator output");
      send(wheels.first,wheels.second); safe_={left_,right_,camera_}; publish_memory();
    } catch(const std::exception& e) { trip(e.what()); }
  }
  block_core::DebugLog debug_{"actuator"};
  block_core::TaskMemory memory_; uint64_t memory_revision_{UINT64_MAX};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr memory_pub_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr memory_service_;
  double left_{},right_{},camera_{},pitch_{},cmd_received_{},grip_received_{},pitch_received_{},fb_time_{},resume_after_{},started_{},timeout_{},speed_{},duration_{};
  double motion_budget_{};
  rclcpp_action::Client<Action>::SharedPtr auto_client_; bool autonomous_pending_{};
  std::map<rclcpp_action::GoalUUID,uint64_t> admissions_; uint64_t generation_{}; bool camera_initialized_{};
  bool healthy_{},reserved_{},motion_active_{}; block_core::Search search_; block_core::SweepControl sweep_ctl_; block_core::ClawOpening claw_open_;
  uint16_t seq_{}; uint64_t task_{},last_task_{};
  std::string state_{"hold"},safety_,rx_buffer_; std::array<double,3> safe_{};
  std::pair<double,double> close_angles_;
  std::optional<AlignmentCommand> cmd_,action_; std::optional<GripperAngles> grip_;
  std::optional<double> health_time_,pitch_time_,cmd_stamp_,close_time_,cam_time_,sweep_time_,motion_time_;
  std::optional<uint64_t> formal_track_; std::optional<uint16_t> fb_seq_;
  std::shared_ptr<GoalHandle> goal_; rclcpp_action::Server<Action>::SharedPtr server_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr tx_,safety_pub_;
  rclcpp::Publisher<TargetTask>::SharedPtr task_pub_; rclcpp::Publisher<TargetSearch>::SharedPtr search_pub_;
  rclcpp::Publisher<AlignmentCommand>::SharedPtr finished_pub_; rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr reset_pub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joints_pub_;
  rclcpp::Subscription<AlignmentCommand>::SharedPtr commands_; rclcpp::Subscription<GripperAngles>::SharedPtr angles_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr health_; rclcpp::Subscription<std_msgs::msg::String>::SharedPtr rx_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joints_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr emergency_,reset_; rclcpp::TimerBase::SharedPtr timer_;
};
int main(int argc,char** argv) {
  rclcpp::init(argc,argv,rclcpp::InitOptions(),rclcpp::SignalHandlerOptions::None); signal(SIGINT,signal_stop); signal(SIGTERM,signal_stop);
  auto node=std::make_shared<Actuator>(); rclcpp::executors::SingleThreadedExecutor executor; executor.add_node(node); const auto parent=getppid();
  while(rclcpp::ok() && !stopping && getppid()==parent) executor.spin_once(std::chrono::milliseconds(50));
  for(int i=0;i<5 && rclcpp::ok();++i) { node->stop(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
  executor.remove_node(node); node.reset(); rclcpp::shutdown(); return 0;
}
