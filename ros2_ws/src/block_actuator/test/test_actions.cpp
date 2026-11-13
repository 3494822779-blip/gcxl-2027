#include <gtest/gtest.h>
#define main actuator_program_main
#include "../src/actuator_node.cpp"
#undef main
#define main task_executor_program_main
#include "../src/task_executor_node.cpp"
#undef main
class Actions : public testing::Test {
 protected:
  static void SetUpTestSuite() { if(!rclcpp::ok()) rclcpp::init(0,nullptr); }
  void SetUp() override {
    if(!rclcpp::ok()) rclcpp::init(0,nullptr);
    actuator=std::make_shared<Actuator>(); client_node=std::make_shared<rclcpp::Node>("action_test_client");
    executor.add_node(actuator); executor.add_node(client_node);
    client=rclcpp_action::create_client<Action>(client_node,"actuator/execute");
    ASSERT_TRUE(client->wait_for_action_server(std::chrono::seconds(3)));
  }
  void TearDown() override { if(relay) executor.remove_node(relay); executor.remove_node(actuator); executor.remove_node(client_node); actuator.reset(); client_node.reset(); }
  template<class Future> bool wait(Future& f,double seconds=3) {
    const double end=steady()+seconds;
    while(steady()<end && f.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) executor.spin_once(std::chrono::milliseconds(10));
    return f.wait_for(std::chrono::milliseconds(0))==std::future_status::ready;
  }
  rclcpp_action::ClientGoalHandle<Action>::SharedPtr send(Action::Goal g) { auto f=client->async_send_goal(g); if(!wait(f)) return {}; return f.get(); }
  std::shared_ptr<TaskExecutor> relay;
  std::shared_ptr<Actuator> actuator; std::shared_ptr<rclcpp::Node> client_node;
  rclcpp::executors::SingleThreadedExecutor executor; rclcpp_action::Client<Action>::SharedPtr client;
};
TEST_F(Actions, RejectsUnknownSubstringNegativeClassAndDiscard) {
  Action::Goal g; g.operation="ove"; EXPECT_FALSE(send(g));
  g.operation="target_grasp";g.has_target_class=true;g.target_class_id=-1;EXPECT_FALSE(send(g));
  g.operation="move";g.discard_after=true;EXPECT_FALSE(send(g));
}
TEST_F(Actions, BusyCancellationAndTimeout) {
  Action::Goal g;g.operation="move";g.distance_m=1;g.speed_mps=.1;
  auto h=send(g);ASSERT_TRUE(h);EXPECT_FALSE(send(g));
  auto cancel=client->async_cancel_goal(h);ASSERT_TRUE(wait(cancel));
  auto result=client->async_get_result(h);ASSERT_TRUE(wait(result));EXPECT_EQ(result.get().code,rclcpp_action::ResultCode::CANCELED);
  g.timeout_sec=.1;h=send(g);ASSERT_TRUE(h);result=client->async_get_result(h);ASSERT_TRUE(wait(result));EXPECT_EQ(result.get().code,rclcpp_action::ResultCode::ABORTED);
}
TEST_F(Actions, ManualGraspCompletesAfterSettleAndRecordsMemory) {
  ASSERT_TRUE(actuator->set_parameter(rclcpp::Parameter("claw_settle_sec",.1)).successful);
  Action::Goal g;g.operation="grasp";auto h=send(g);ASSERT_TRUE(h);
  auto result=client->async_get_result(h);ASSERT_TRUE(wait(result));EXPECT_EQ(result.get().code,rclcpp_action::ResultCode::SUCCEEDED);
  EXPECT_TRUE(result.get().result->success);EXPECT_NE(result.get().result->message.find("unconfirmed"),std::string::npos);
  auto service=client_node->create_client<std_srvs::srv::Trigger>("robot/memory/get");ASSERT_TRUE(service->wait_for_service(std::chrono::seconds(1)));
  auto response=service->async_send_request(std::make_shared<std_srvs::srv::Trigger::Request>());ASSERT_TRUE(wait(response));
  EXPECT_NE(response.get()->message.find("commanded_hold"),std::string::npos);
}
TEST_F(Actions, RejectsIllegalOnlineParameters) {
  EXPECT_FALSE(actuator->set_parameter(rclcpp::Parameter("max_wheel_speed",-1.)).successful);
  EXPECT_FALSE(actuator->set_parameter(rclcpp::Parameter("discard_enabled",true)).successful);
}
TEST_F(Actions, EmergencyStopLatchesUntilExplicitReset) {
  auto emergency=client_node->create_client<std_srvs::srv::Trigger>("robot/emergency_stop");
  ASSERT_TRUE(emergency->wait_for_service(std::chrono::seconds(1)));
  auto response=emergency->async_send_request(std::make_shared<std_srvs::srv::Trigger::Request>());ASSERT_TRUE(wait(response));ASSERT_TRUE(response.get()->success);
  Action::Goal g;g.operation="move";g.distance_m=.01;EXPECT_FALSE(send(g));
  auto reset=client_node->create_client<std_srvs::srv::Trigger>("robot/reset_safety");ASSERT_TRUE(reset->wait_for_service(std::chrono::seconds(1)));
  auto reset_response=reset->async_send_request(std::make_shared<std_srvs::srv::Trigger::Request>());ASSERT_TRUE(wait(reset_response));ASSERT_TRUE(reset_response.get()->success);
  auto h=send(g);ASSERT_TRUE(h);auto result=client->async_get_result(h);ASSERT_TRUE(wait(result));EXPECT_TRUE(result.get().result->success);
}
TEST_F(Actions, TargetGraspConsumesOpeningAndCompletesExactlyOnce) {
  ASSERT_TRUE(actuator->set_parameter(rclcpp::Parameter("claw_settle_sec",.02)).successful);
  uint64_t task_id=0;
  auto tasks=client_node->create_subscription<TargetTask>("task/target_grasp",rclcpp::QoS(1).transient_local(),[&](TargetTask::ConstSharedPtr m){if(m->active) task_id=m->task_id;});
  auto commands=client_node->create_publisher<AlignmentCommand>("alignment/command",10);
  auto gripper=client_node->create_publisher<GripperAngles>("gripper/angles",10);
  Action::Goal g;g.operation="target_grasp";g.has_target_class=true;g.target_class_id=2;g.timeout_sec=3;
  auto h=send(g);ASSERT_TRUE(h);auto result=client->async_get_result(h);
  const double deadline=steady()+2;
  while(steady()<deadline && result.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready) {
    executor.spin_once(std::chrono::milliseconds(10)); if(!task_id) continue;
    AlignmentCommand c;c.header.stamp=client_node->now();c.valid=true;c.track_id=8;c.class_id=2;c.first_grasp_id=task_id;c.valid_for_sec=.25;c.open_allowed=c.grasp_request=c.aligned=true;
    GripperAngles a;a.header=c.header;a.track_id=8;a.angles_ready=true;a.bounds_valid=true;a.valid_for_sec=.25;a.left_angle_rad=.3;a.right_angle_rad=.4;
    gripper->publish(a);commands->publish(c);
  }
  ASSERT_TRUE(wait(result));EXPECT_TRUE(result.get().result->success);
  auto service=client_node->create_client<std_srvs::srv::Trigger>("robot/memory/get");ASSERT_TRUE(service->wait_for_service(std::chrono::seconds(1)));
  auto response=service->async_send_request(std::make_shared<std_srvs::srv::Trigger::Request>());ASSERT_TRUE(wait(response));
  Json::Value memory;std::istringstream text(response.get()->message);text>>memory;
  ASSERT_EQ(memory["history"]["grasps"].size(),1u);EXPECT_EQ(memory["context"]["held"]["state"],"commanded_hold");
}

TEST_F(Actions, PublicActionReturnsActualActuatorTimeout) {
  relay=std::make_shared<TaskExecutor>();executor.add_node(relay);
  client=rclcpp_action::create_client<Action>(client_node,"robot/execute");
  ASSERT_TRUE(client->wait_for_action_server(std::chrono::seconds(2)));
  Action::Goal g;g.operation="move";g.distance_m=1;g.timeout_sec=.1;
  auto h=send(g);ASSERT_TRUE(h);auto result=client->async_get_result(h);ASSERT_TRUE(wait(result));
  EXPECT_EQ(result.get().code,rclcpp_action::ResultCode::ABORTED);
  EXPECT_FALSE(result.get().result->success);EXPECT_NE(result.get().result->message.find("timeout"),std::string::npos);
}
