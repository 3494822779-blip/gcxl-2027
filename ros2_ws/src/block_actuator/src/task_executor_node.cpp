#include <set>
#include <cmath>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <block_interfaces/action/robot_task.hpp>
#include <map>
// Public action endpoint forwards lifecycle events to the sole actuator owner.
// Never fabricate completion from elapsed time or from acceptance alone.
class TaskExecutor final : public rclcpp::Node {
  using Action=block_interfaces::action::RobotTask;
  using ServerHandle=rclcpp_action::ServerGoalHandle<Action>;
  using ClientHandle=rclcpp_action::ClientGoalHandle<Action>;
  struct Pending { std::shared_ptr<ServerHandle> external; ClientHandle::SharedPtr internal; std::chrono::steady_clock::time_point deadline; };
 public:
  TaskExecutor():Node("task_executor") {
    client_=rclcpp_action::create_client<Action>(this,"actuator/execute");
    server_=rclcpp_action::create_server<Action>(this,"robot/execute",
      [this](const rclcpp_action::GoalUUID&,std::shared_ptr<const Action::Goal> g){
        const std::set<std::string> operations{"align","target_grasp","approach_grasp","first_grasp","grasp","move","turn","claw","camera","stop"};
        const bool target=g->operation=="target_grasp" || g->operation=="approach_grasp" || g->operation=="first_grasp";
        if(!client_->action_server_is_ready() || !operations.count(g->operation) || g->discard_after ||
          !std::isfinite(g->timeout_sec) || !std::isfinite(g->distance_m) || !std::isfinite(g->angle_deg) || !std::isfinite(g->speed_mps) ||
          g->timeout_sec<0 || g->speed_mps<0 || (target && (!g->has_target_class || g->target_class_id<0)) ||
          (reserved_ && g->operation!="stop")) return rclcpp_action::GoalResponse::REJECT;
        if(g->operation!="stop") reserved_=true;
        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
      },
      [this](std::shared_ptr<ServerHandle> h){
        auto it=pending_.find(h.get());
        if(it!=pending_.end() && it->second->internal) client_->async_cancel_goal(it->second->internal);
        return rclcpp_action::CancelResponse::ACCEPT;
      },[this](std::shared_ptr<ServerHandle> h){start(h);});
    watchdog_=create_wall_timer(std::chrono::milliseconds(50),[this]{
      std::vector<std::shared_ptr<Pending>> expired;
      for(const auto& entry:pending_) if(std::chrono::steady_clock::now()>=entry.second->deadline) expired.push_back(entry.second);
      for(const auto& request:expired) {
        if(request->internal) client_->async_cancel_goal(request->internal);
        auto result=std::make_shared<Action::Result>(); result->message="actuator response timeout; task completion unconfirmed";
        finish(request,result,rclcpp_action::ResultCode::ABORTED);
      }
    });
  }
 private:
  void start(const std::shared_ptr<ServerHandle>& h) {
    auto request=std::make_shared<Pending>(); request->external=h;
    const auto g=h->get_goal();
    const bool target=g->operation=="target_grasp" || g->operation=="approach_grasp" || g->operation=="first_grasp";
    const double timeout=(g->timeout_sec>0?g->timeout_sec:target?180.:60.)+1.;
    request->deadline=std::chrono::steady_clock::now()+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeout));
    pending_[h.get()]=request;
    rclcpp_action::Client<Action>::SendGoalOptions options;
    options.goal_response_callback=[this,request](ClientHandle::SharedPtr handle){
      request->internal=handle;
      if(!handle) { auto result=std::make_shared<Action::Result>(); result->message="actuator rejected task"; finish(request,result,rclcpp_action::ResultCode::ABORTED); }
      else if(request->external->is_canceling()) client_->async_cancel_goal(handle);
    };
    options.feedback_callback=[request](ClientHandle::SharedPtr,std::shared_ptr<const Action::Feedback> feedback){
      if(request->external->is_active()) request->external->publish_feedback(std::make_shared<Action::Feedback>(*feedback));
    };
    options.result_callback=[this,request](const ClientHandle::WrappedResult& result){finish(request,result.result,result.code);};
    client_->async_send_goal(*h->get_goal(),options);
  }
  void finish(const std::shared_ptr<Pending>& p,std::shared_ptr<Action::Result> result,rclcpp_action::ResultCode code) {
    if(!pending_.count(p->external.get())) return;
    if(!result) { result=std::make_shared<Action::Result>(); result->message="actuator result unavailable"; }
    if(p->external->is_canceling()) p->external->canceled(result);
    else if(code==rclcpp_action::ResultCode::SUCCEEDED) p->external->succeed(result);
    else p->external->abort(result);
    if(p->external->get_goal()->operation!="stop") reserved_=false;
    pending_.erase(p->external.get());
  }
  rclcpp::TimerBase::SharedPtr watchdog_;
  bool reserved_{};
  std::map<ServerHandle*,std::shared_ptr<Pending>> pending_;
  rclcpp_action::Client<Action>::SharedPtr client_;
  rclcpp_action::Server<Action>::SharedPtr server_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<TaskExecutor>()); rclcpp::shutdown(); return 0; }
