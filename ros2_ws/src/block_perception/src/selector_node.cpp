#include <block_perception/parameter_validation.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/bool.hpp>
#include <block_interfaces/msg/frame_metadata.hpp>
#include <block_interfaces/msg/target.hpp>
#include <block_interfaces/msg/target_task.hpp>
#include <block_interfaces/msg/target_search.hpp>
#include <block_core/core.hpp>
using namespace block_interfaces::msg;
class Selector final : public rclcpp::Node {
 public:
  Selector():Node("selector"),lock_(.3,3.) {
    declare_parameter("rebind_iou",.3); declare_parameter("rebind_timeout",3.);
    declare_parameter("search_rebind_iou",.1); reset_lock();
    parameter_guard_=parameter_guard(*this);
    pub_=create_publisher<Target>("target/raw",10);
    frames_=create_subscription<FrameMetadata>("tracks_meta",rclcpp::SensorDataQoS(),[this](FrameMetadata::ConstSharedPtr m){frame(*m);});
    tasks_=create_subscription<TargetTask>("task/target_grasp",rclcpp::QoS(1).transient_local(),[this](TargetTask::ConstSharedPtr m){
      if(m->active) {
        if(m->task_id<=last_task_) return;
        last_task_=task_=m->task_id; cls_=m->target_class_id; active_=true;
        search_.reset(); reset_lock(); last_stamp_=task_*1e-9;
      } else if(m->task_id==task_) { active_=false; lock_=block_core::Lock(0.); }
      Target out; out.header.stamp=now(); pub_->publish(out);
    });
    resets_=create_subscription<std_msgs::msg::Bool>("target/reset",10,[this](std_msgs::msg::Bool::ConstSharedPtr m){
      if(!m->data) return;
      task_=0; active_=false; last_stamp_.reset(); search_.reset(); reset_lock();
      Target out; out.header.stamp=now(); pub_->publish(out);
    });
    searches_=create_subscription<TargetSearch>("task/target_search",10,[this](TargetSearch::ConstSharedPtr m){
      if(!active_ || m->task_id!=task_ || (search_ && rclcpp::Time(m->header.stamp)<=rclcpp::Time(search_->header.stamp))) return;
      search_=*m; search_received_=std::chrono::steady_clock::now();
    });
  }
 private:
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard_;
  void reset_lock() { lock_=block_core::Lock(get_parameter("rebind_iou").as_double(),get_parameter("rebind_timeout").as_double()); }
  void frame(const FrameMetadata& m) {
    const double t=rclcpp::Time(m.header.stamp).seconds();
    if(last_stamp_ && t<=*last_stamp_) return;
    last_stamp_=t; Target out; out.header=m.header;
    if(task_ && !active_) { pub_->publish(out); return; }
    std::vector<block_core::Box> boxes; std::set<uint64_t> known;
    for(const auto& b:m.boxes) { boxes.push_back({b.track_id,b.class_id,b.confidence,b.x,b.y,b.width,b.height,b.observed}); known.insert(b.track_id); }
    const auto eligible=active_?block_core::candidates(boxes,cls_):boxes;
    bool searching=false;
    if(active_ && search_ && search_->active) {
      const double age=t-rclcpp::Time(search_->header.stamp).seconds();
      const double received=std::chrono::duration<double>(std::chrono::steady_clock::now()-search_received_).count();
      searching=age>=0 && age<=.25 && received>=0 && received<=.25;
    }
    if(!active_) lock_.configure(get_parameter("rebind_iou").as_double(),get_parameter("rebind_timeout").as_double());
    const auto* b=lock_.select(eligible,t,searching,get_parameter("search_rebind_iou").as_double(),known,!searching || search_->stationary);
    out.image_width=m.width; out.image_height=m.height; out.locked=lock_.locked(); out.track_id=lock_.id();
    out.observed=out.valid=b!=nullptr; out.first_grasp_id=task_;
    if(b) {
      out.class_id=b->class_id; out.raw_x=out.x=b->x+b->width/2; out.raw_y=out.y=b->y+b->height/2; out.last_observed=m.header.stamp;
      if(active_) if(auto obstacle=block_core::obstacle_for(*b,boxes)) {
        out.obstacle_id=obstacle->box.track_id; out.sweep_side=obstacle->side;
        out.obstacle_overlap=obstacle->overlap; out.obstacle_near=obstacle->near;
      }
    }
    pub_->publish(out);
  }
  block_core::Lock lock_;
  uint64_t task_{},last_task_{}; int cls_{}; bool active_{};
  std::optional<double> last_stamp_; std::optional<TargetSearch> search_;
  std::chrono::steady_clock::time_point search_received_;
  rclcpp::Publisher<Target>::SharedPtr pub_;
  rclcpp::Subscription<FrameMetadata>::SharedPtr frames_;
  rclcpp::Subscription<TargetTask>::SharedPtr tasks_;
  rclcpp::Subscription<TargetSearch>::SharedPtr searches_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr resets_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<Selector>()); rclcpp::shutdown(); return 0; }
