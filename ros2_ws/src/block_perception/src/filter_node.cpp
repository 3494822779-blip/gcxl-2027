#include <block_perception/parameter_validation.hpp>
#include <rclcpp/rclcpp.hpp>
#include <block_interfaces/msg/target.hpp>
#include <block_core/core.hpp>
using block_interfaces::msg::Target;
class Filter final : public rclcpp::Node {
 public:
  Filter():Node("filter") {
    declare_parameter("process_noise",100.); declare_parameter("measurement_noise",16.);
    declare_parameter("initial_variance",100.); declare_parameter("prediction_timeout",.4);
    parameter_guard_=parameter_guard(*this);
    pub_=create_publisher<Target>("target/filtered",10);
    sub_=create_subscription<Target>("target/raw",10,[this](Target::ConstSharedPtr m){target(*m);});
  }
 private:
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_guard_;
  void target(const Target& m) {
    if(!m.locked || !identity_ || *identity_!=m.track_id) { kf_.reset(); last_.reset(); }
    identity_=m.locked?std::optional<uint64_t>(m.track_id):std::nullopt;
    Target out=m;
    if(m.locked) {
      const double t=rclcpp::Time(m.header.stamp).seconds();
      if(kf_.time() && t<=*kf_.time()) return;
      const double timeout=get_parameter("prediction_timeout").as_double();
      if(m.observed && kf_.last_seen() && t-*kf_.last_seen()>timeout) kf_.reset();
      const std::vector<double> measurement{m.raw_x,m.raw_y};
      const auto xy=kf_.step(t,m.observed?&measurement:nullptr,get_parameter("process_noise").as_double(),
        get_parameter("measurement_noise").as_double(),get_parameter("initial_variance").as_double());
      if(m.observed) last_=m;
      else if(last_) { out.raw_x=last_->raw_x; out.raw_y=last_->raw_y; out.class_id=last_->class_id; out.last_observed=last_->last_observed; }
      out.valid=!xy.empty() && kf_.last_seen() && t-*kf_.last_seen()<=timeout;
      if(!xy.empty()) { out.x=xy[0]; out.y=xy[1]; }
    }
    pub_->publish(out);
  }
  block_core::CVFilter kf_; std::optional<uint64_t> identity_; std::optional<Target> last_;
  rclcpp::Publisher<Target>::SharedPtr pub_; rclcpp::Subscription<Target>::SharedPtr sub_;
};
int main(int argc,char** argv) { rclcpp::init(argc,argv); rclcpp::spin(std::make_shared<Filter>()); rclcpp::shutdown(); return 0; }
