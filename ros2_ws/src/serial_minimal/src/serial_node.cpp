#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/bool.hpp>
#include <block_core/tx_buffer.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <map>
namespace {
volatile sig_atomic_t stopping=0;
void stop_handler(int) { stopping=1; }
double steady() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
}
class SerialNode final : public rclcpp::Node {
 public:
  explicit SerialNode(const rclcpp::NodeOptions& options=rclcpp::NodeOptions()):Node("serial_node",options),tx_(declare_parameter("write_timeout",.1)) {
    const auto port=declare_parameter("port",std::string("/dev/ttyUSB0"));
    const int baud=declare_parameter("baud",115200);
    const double period=declare_parameter("poll_period",.01);
    declare_parameter("shutdown_grace",.5);
    const std::map<int,speed_t> rates{{9600,B9600},{19200,B19200},{38400,B38400},{57600,B57600},{115200,B115200}};
    if(!rates.count(baud) || !std::isfinite(period) || period<=0) throw std::invalid_argument("invalid baud/poll period");
    fd_=open(port.c_str(),O_RDWR|O_NOCTTY|O_NONBLOCK);
    if(fd_<0) throw std::runtime_error(std::string("open serial: ")+strerror(errno));
    termios attrs{};
    if(tcgetattr(fd_,&attrs)<0) fail_open();
    cfmakeraw(&attrs); cfsetispeed(&attrs,rates.at(baud)); cfsetospeed(&attrs,rates.at(baud));
    if(tcsetattr(fd_,TCSANOW,&attrs)<0) fail_open();
    rx_=create_publisher<std_msgs::msg::String>("serial_rx",10);
    health_=create_publisher<std_msgs::msg::Bool>("serial/healthy",rclcpp::QoS(1).transient_local()); health(false);
    sub_=create_subscription<std_msgs::msg::String>("serial_tx",1,[this](std_msgs::msg::String::ConstSharedPtr m){tx_.enqueue(m->data+'\n',steady()); flush();});
    timer_=create_wall_timer(std::chrono::duration<double>(period),[this]{poll();});
  }
  ~SerialNode() override { if(fd_>=0) close(fd_); }
 private:
  [[noreturn]] void fail_open() { const auto message=std::string(strerror(errno)); close(fd_); fd_=-1; throw std::runtime_error(message); }
  void health(bool value) { std_msgs::msg::Bool m; m.data=value; health_->publish(m); }
  void flush() {
    try { tx_.flush(steady(),[this](const std::string& bytes)->long {
      const auto n=write(fd_,bytes.data(),bytes.size());
      if(n<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) return -1;
      if(n<0) throw std::runtime_error(strerror(errno));
      return n;
    }); } catch(const std::exception& e) { health(false); RCLCPP_ERROR_THROTTLE(get_logger(),*get_clock(),1000,"Write failed: %s",e.what()); }
  }
  void poll() {
    char bytes[4096]; const auto count=read(fd_,bytes,sizeof(bytes));
    if(count<0) read_ok_=errno==EAGAIN || errno==EWOULDBLOCK;
    else { read_ok_=count>0; if(count>0) { std_msgs::msg::String m; m.data.assign(bytes,count); rx_->publish(m); } }
    flush(); const auto completed=tx_.last_complete(); const double t=steady();
    health(read_ok_ && completed && t-*completed>=0 && t-*completed<=.15);
  }
  int fd_{-1}; bool read_ok_{true}; block_core::TxBuffer tx_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr rx_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr health_;
  rclcpp::TimerBase::SharedPtr timer_;
};
int main(int argc,char** argv) {
  rclcpp::init(argc,argv,rclcpp::InitOptions(),rclcpp::SignalHandlerOptions::None);
  signal(SIGINT,stop_handler); signal(SIGTERM,stop_handler);
  auto node=std::make_shared<SerialNode>(); rclcpp::executors::SingleThreadedExecutor executor; executor.add_node(node);
  const auto parent=getppid();
  while(rclcpp::ok() && !stopping && getppid()==parent) executor.spin_once(std::chrono::milliseconds(50));
  const double deadline=steady()+std::max(0.,node->get_parameter("shutdown_grace").as_double());
  while(rclcpp::ok() && steady()<deadline) executor.spin_once(std::chrono::milliseconds(20));
  executor.remove_node(node); node.reset(); rclcpp::shutdown(); return 0;
}
