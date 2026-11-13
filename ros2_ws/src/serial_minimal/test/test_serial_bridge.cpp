#include <gtest/gtest.h>
#include <pty.h>
#define main serial_program_main
#include "../src/serial_node.cpp"
#undef main
TEST(SerialBridge, PseudoTerminalTopicsHealthAndDisconnect) {
  if(!rclcpp::ok()) rclcpp::init(0,nullptr);
  int master=-1,slave=-1;char port[128]{};ASSERT_EQ(openpty(&master,&slave,port,nullptr,nullptr),0);
  fcntl(master,F_SETFL,O_NONBLOCK);
  rclcpp::NodeOptions options;options.parameter_overrides({rclcpp::Parameter("port",std::string(port))});
  auto bridge=std::make_shared<SerialNode>(options);close(slave);
  auto client=std::make_shared<rclcpp::Node>("serial_bridge_test");rclcpp::executors::SingleThreadedExecutor executor;executor.add_node(bridge);executor.add_node(client);
  auto tx=client->create_publisher<std_msgs::msg::String>("serial_tx",10);
  std::string feedback;bool healthy=false;
  auto rx=client->create_subscription<std_msgs::msg::String>("serial_rx",10,[&](std_msgs::msg::String::ConstSharedPtr m){feedback+=m->data;});
  auto status=client->create_subscription<std_msgs::msg::Bool>("serial/healthy",rclcpp::QoS(1).transient_local(),[&](std_msgs::msg::Bool::ConstSharedPtr m){healthy=m->data;});
  auto wait=[&](const std::function<bool()>& done){const double until=steady()+2;while(!done() && steady()<until)executor.spin_once(std::chrono::milliseconds(5));return done();};
  ASSERT_TRUE(wait([&]{return tx->get_subscription_count()>0;}));
  std_msgs::msg::String command;command.data="$CMD,test";tx->publish(command);
  ASSERT_TRUE(wait([&]{return healthy;}));char buffer[64]{};const auto count=read(master,buffer,sizeof(buffer));ASSERT_GT(count,0);EXPECT_EQ(std::string(buffer,count),"$CMD,test\n");
  ASSERT_EQ(write(master,"$FB,1,",6),6);ASSERT_EQ(write(master,"20*00\n",6),6);
  ASSERT_TRUE(wait([&]{return feedback=="$FB,1,20*00\n";}));
  close(master);ASSERT_TRUE(wait([&]{return !healthy;}));
  executor.remove_node(bridge);executor.remove_node(client);
}
