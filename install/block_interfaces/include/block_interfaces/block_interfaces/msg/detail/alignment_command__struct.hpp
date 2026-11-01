// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__block_interfaces__msg__AlignmentCommand __attribute__((deprecated))
#else
# define DEPRECATED__block_interfaces__msg__AlignmentCommand __declspec(deprecated)
#endif

namespace block_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct AlignmentCommand_
{
  using Type = AlignmentCommand_<ContainerAllocator>;

  explicit AlignmentCommand_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->valid = false;
      this->predicted = false;
      this->error_x = 0.0;
      this->error_y = 0.0;
      this->yaw_rate = 0.0;
      this->pitch_rate = 0.0;
      this->aligned = false;
      this->grasp_request = false;
      this->valid_for_sec = 0.0;
      this->reason = "";
    }
  }

  explicit AlignmentCommand_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_alloc, _init),
    reason(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->valid = false;
      this->predicted = false;
      this->error_x = 0.0;
      this->error_y = 0.0;
      this->yaw_rate = 0.0;
      this->pitch_rate = 0.0;
      this->aligned = false;
      this->grasp_request = false;
      this->valid_for_sec = 0.0;
      this->reason = "";
    }
  }

  // field types and members
  using _header_type =
    std_msgs::msg::Header_<ContainerAllocator>;
  _header_type header;
  using _track_id_type =
    uint64_t;
  _track_id_type track_id;
  using _valid_type =
    bool;
  _valid_type valid;
  using _predicted_type =
    bool;
  _predicted_type predicted;
  using _error_x_type =
    double;
  _error_x_type error_x;
  using _error_y_type =
    double;
  _error_y_type error_y;
  using _yaw_rate_type =
    double;
  _yaw_rate_type yaw_rate;
  using _pitch_rate_type =
    double;
  _pitch_rate_type pitch_rate;
  using _aligned_type =
    bool;
  _aligned_type aligned;
  using _grasp_request_type =
    bool;
  _grasp_request_type grasp_request;
  using _valid_for_sec_type =
    double;
  _valid_for_sec_type valid_for_sec;
  using _reason_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _reason_type reason;

  // setters for named parameter idiom
  Type & set__header(
    const std_msgs::msg::Header_<ContainerAllocator> & _arg)
  {
    this->header = _arg;
    return *this;
  }
  Type & set__track_id(
    const uint64_t & _arg)
  {
    this->track_id = _arg;
    return *this;
  }
  Type & set__valid(
    const bool & _arg)
  {
    this->valid = _arg;
    return *this;
  }
  Type & set__predicted(
    const bool & _arg)
  {
    this->predicted = _arg;
    return *this;
  }
  Type & set__error_x(
    const double & _arg)
  {
    this->error_x = _arg;
    return *this;
  }
  Type & set__error_y(
    const double & _arg)
  {
    this->error_y = _arg;
    return *this;
  }
  Type & set__yaw_rate(
    const double & _arg)
  {
    this->yaw_rate = _arg;
    return *this;
  }
  Type & set__pitch_rate(
    const double & _arg)
  {
    this->pitch_rate = _arg;
    return *this;
  }
  Type & set__aligned(
    const bool & _arg)
  {
    this->aligned = _arg;
    return *this;
  }
  Type & set__grasp_request(
    const bool & _arg)
  {
    this->grasp_request = _arg;
    return *this;
  }
  Type & set__valid_for_sec(
    const double & _arg)
  {
    this->valid_for_sec = _arg;
    return *this;
  }
  Type & set__reason(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->reason = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    block_interfaces::msg::AlignmentCommand_<ContainerAllocator> *;
  using ConstRawPtr =
    const block_interfaces::msg::AlignmentCommand_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::AlignmentCommand_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::AlignmentCommand_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__block_interfaces__msg__AlignmentCommand
    std::shared_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__block_interfaces__msg__AlignmentCommand
    std::shared_ptr<block_interfaces::msg::AlignmentCommand_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const AlignmentCommand_ & other) const
  {
    if (this->header != other.header) {
      return false;
    }
    if (this->track_id != other.track_id) {
      return false;
    }
    if (this->valid != other.valid) {
      return false;
    }
    if (this->predicted != other.predicted) {
      return false;
    }
    if (this->error_x != other.error_x) {
      return false;
    }
    if (this->error_y != other.error_y) {
      return false;
    }
    if (this->yaw_rate != other.yaw_rate) {
      return false;
    }
    if (this->pitch_rate != other.pitch_rate) {
      return false;
    }
    if (this->aligned != other.aligned) {
      return false;
    }
    if (this->grasp_request != other.grasp_request) {
      return false;
    }
    if (this->valid_for_sec != other.valid_for_sec) {
      return false;
    }
    if (this->reason != other.reason) {
      return false;
    }
    return true;
  }
  bool operator!=(const AlignmentCommand_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct AlignmentCommand_

// alias to use template instance with default allocator
using AlignmentCommand =
  block_interfaces::msg::AlignmentCommand_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_HPP_
