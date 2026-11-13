#include <gtest/gtest.h>
#define main selector_main
#include "../src/selector_node.cpp"
#undef main
#define main filter_main
#include "../src/filter_node.cpp"
#undef main
#define main gripper_main
#include "../src/gripper_node.cpp"
#undef main
#define main alignment_main
#include "../src/alignment_node.cpp"
#undef main
class Nodes : public testing::Test {
 protected:
  static void SetUpTestSuite() { if(!rclcpp::ok()) rclcpp::init(0,nullptr); }
  void SetUp() override {
    if(!rclcpp::ok()) rclcpp::init(0,nullptr);
    client=std::make_shared<rclcpp::Node>("node_test_client"); executor.add_node(client);
  }
  void TearDown() override { for(auto& n:nodes) executor.remove_node(n); executor.remove_node(client); }
  template<class T> std::shared_ptr<T> add() { auto n=std::make_shared<T>();nodes.push_back(n);executor.add_node(n);return n; }
  template<class Predicate> bool wait(Predicate predicate,double timeout=3.) {
    const auto end=std::chrono::steady_clock::now()+std::chrono::duration<double>(timeout);
    while(!predicate() && std::chrono::steady_clock::now()<end) executor.spin_once(std::chrono::milliseconds(5));
    return predicate();
  }
  rclcpp::executors::SingleThreadedExecutor executor; std::shared_ptr<rclcpp::Node> client; std::vector<std::shared_ptr<rclcpp::Node>> nodes;
};
TEST_F(Nodes, FilterSuppressesDuplicatesAndExpiresPrediction) {
  add<Filter>(); std::vector<Target> results;
  auto sub=client->create_subscription<Target>("target/filtered",10,[&](Target::ConstSharedPtr m){results.push_back(*m);});
  auto pub=client->create_publisher<Target>("target/raw",10); ASSERT_TRUE(wait([&]{return pub->get_subscription_count()>0;}));
  Target t; t.header.stamp=rclcpp::Time(1000000000LL);t.locked=t.observed=t.valid=true;t.track_id=1;t.raw_x=100;t.raw_y=200;t.last_observed=t.header.stamp;
  pub->publish(t);ASSERT_TRUE(wait([&]{return results.size()==1;}));pub->publish(t);
  wait([]{return false;},.05);EXPECT_EQ(results.size(),1u);
  t.header.stamp=rclcpp::Time(1600000000LL);t.observed=false;pub->publish(t);
  ASSERT_TRUE(wait([&]{return results.size()==2;}));EXPECT_FALSE(results.back().valid);EXPECT_EQ(results.back().raw_x,100);
}
TEST_F(Nodes, GripperRequiresSameFrameAndActualEdges) {
  auto gripper=add<Gripper>(); ASSERT_TRUE(gripper->set_parameter(rclcpp::Parameter("input_timeout",2.)).successful);
  std::vector<GripperAngles> results;auto sub=client->create_subscription<GripperAngles>("gripper/angles",10,[&](GripperAngles::ConstSharedPtr m){results.push_back(*m);});
  auto tp=client->create_publisher<Target>("target/raw",10);auto fp=client->create_publisher<FrameMetadata>("tracks_meta",rclcpp::SensorDataQoS());
  ASSERT_TRUE(wait([&]{return tp->get_subscription_count()>0 && fp->get_subscription_count()>0;}));
  Target t;t.header.stamp=client->now();t.header.frame_id="camera";t.locked=t.valid=t.observed=true;t.track_id=8;t.image_width=640;t.image_height=480;
  tp->publish(t);ASSERT_TRUE(wait([&]{return !results.empty();}));
  FrameMetadata f;f.header=t.header;f.width=640;f.height=480;Box b;b.track_id=8;b.observed=true;b.x=100;b.width=200;b.height=40;f.boxes.push_back(b);
  f.header.frame_id="other";fp->publish(f);wait([]{return false;},.05);
  EXPECT_FALSE(std::any_of(results.begin(),results.end(),[](auto& g){return g.angles_ready;}));
  f.header=t.header;fp->publish(f);ASSERT_TRUE(wait([&]{return std::any_of(results.begin(),results.end(),[](auto& g){return g.angles_ready;});}));
  const auto g=*std::find_if(results.begin(),results.end(),[](auto& g){return g.angles_ready;});
  EXPECT_EQ(g.left_edge_px,100);EXPECT_EQ(g.right_edge_px,300);
  EXPECT_NEAR(g.left_angle_rad,block_core::gripper_angles(100,300,640,480,{}).first,1e-8);
}
TEST_F(Nodes, ExplicitClassTaskAndOldRequestIgnored) {
  add<Selector>();std::vector<Target> results;auto sub=client->create_subscription<Target>("target/raw",10,[&](Target::ConstSharedPtr m){results.push_back(*m);});
  auto tasks=client->create_publisher<TargetTask>("task/target_grasp",rclcpp::QoS(1).transient_local());
  auto frames=client->create_publisher<FrameMetadata>("tracks_meta",rclcpp::SensorDataQoS());
  ASSERT_TRUE(wait([&]{return tasks->get_subscription_count()>0 && frames->get_subscription_count()>0;}));
  TargetTask task;task.task_id=1;task.target_class_id=2;task.active=true;tasks->publish(task);
  ASSERT_TRUE(wait([&]{return !results.empty();}));
  FrameMetadata frame;frame.header.stamp=client->now();frame.width=640;frame.height=480;
  Box a;a.track_id=1;a.class_id=1;a.observed=true;a.width=100;a.height=200;a.confidence=.9;
  Box b=a;b.track_id=2;b.class_id=2;b.height=100;frame.boxes={a,b};frames->publish(frame);
  ASSERT_TRUE(wait([&]{return !results.empty() && results.back().valid;}));EXPECT_EQ(results.back().class_id,2);EXPECT_EQ(results.back().track_id,2u);
  task.target_class_id=1;tasks->publish(task);wait([]{return false;},.05);frame.header.stamp=client->now();frames->publish(frame);
  ASSERT_TRUE(wait([&]{return results.back().header.stamp==frame.header.stamp;}));EXPECT_EQ(results.back().class_id,2);
}
TEST_F(Nodes, AlignmentRetainsTaskIdentityWhenTargetExpires) {
  auto alignment=add<Alignment>();std::vector<AlignmentCommand> results;
  auto sub=client->create_subscription<AlignmentCommand>("alignment/command",10,[&](AlignmentCommand::ConstSharedPtr m){results.push_back(*m);});
  auto pub=client->create_publisher<Target>("target/filtered",10);ASSERT_TRUE(wait([&]{return pub->get_subscription_count()>0;}));
  Target t;t.header.stamp=client->now();t.locked=t.valid=t.observed=true;t.track_id=4;t.first_grasp_id=7;t.class_id=2;t.image_width=640;t.image_height=480;t.x=320;t.y=240;pub->publish(t);
  ASSERT_TRUE(wait([&]{return !results.empty() && results.back().grasp_request;}));
  ASSERT_TRUE(wait([&]{return !results.empty() && !results.back().valid && results.back().first_grasp_id==7;}));
  EXPECT_EQ(results.back().forward_speed,0);EXPECT_EQ(results.back().track_id,4u);
}
