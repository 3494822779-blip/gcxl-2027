// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_HPP_

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
# define DEPRECATED__block_interfaces__msg__GripperAngles __attribute__((deprecated))
#else
# define DEPRECATED__block_interfaces__msg__GripperAngles __declspec(deprecated)
#endif

namespace block_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct GripperAngles_
{
  using Type = GripperAngles_<ContainerAllocator>;

  explicit GripperAngles_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->image_width = 0ul;
      this->image_height = 0ul;
      this->bounds_valid = false;
      this->left_edge_px = 0.0;
      this->right_edge_px = 0.0;
      this->angles_ready = false;
      this->left_angle_rad = 0.0;
      this->right_angle_rad = 0.0;
      this->valid_for_sec = 0.0;
      this->reason = "";
    }
  }

  explicit GripperAngles_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_alloc, _init),
    reason(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->image_width = 0ul;
      this->image_height = 0ul;
      this->bounds_valid = false;
      this->left_edge_px = 0.0;
      this->right_edge_px = 0.0;
      this->angles_ready = false;
      this->left_angle_rad = 0.0;
      this->right_angle_rad = 0.0;
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
  using _image_width_type =
    uint32_t;
  _image_width_type image_width;
  using _image_height_type =
    uint32_t;
  _image_height_type image_height;
  using _bounds_valid_type =
    bool;
  _bounds_valid_type bounds_valid;
  using _left_edge_px_type =
    double;
  _left_edge_px_type left_edge_px;
  using _right_edge_px_type =
    double;
  _right_edge_px_type right_edge_px;
  using _angles_ready_type =
    bool;
  _angles_ready_type angles_ready;
  using _left_angle_rad_type =
    double;
  _left_angle_rad_type left_angle_rad;
  using _right_angle_rad_type =
    double;
  _right_angle_rad_type right_angle_rad;
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
  Type & set__image_width(
    const uint32_t & _arg)
  {
    this->image_width = _arg;
    return *this;
  }
  Type & set__image_height(
    const uint32_t & _arg)
  {
    this->image_height = _arg;
    return *this;
  }
  Type & set__bounds_valid(
    const bool & _arg)
  {
    this->bounds_valid = _arg;
    return *this;
  }
  Type & set__left_edge_px(
    const double & _arg)
  {
    this->left_edge_px = _arg;
    return *this;
  }
  Type & set__right_edge_px(
    const double & _arg)
  {
    this->right_edge_px = _arg;
    return *this;
  }
  Type & set__angles_ready(
    const bool & _arg)
  {
    this->angles_ready = _arg;
    return *this;
  }
  Type & set__left_angle_rad(
    const double & _arg)
  {
    this->left_angle_rad = _arg;
    return *this;
  }
  Type & set__right_angle_rad(
    const double & _arg)
  {
    this->right_angle_rad = _arg;
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
    block_interfaces::msg::GripperAngles_<ContainerAllocator> *;
  using ConstRawPtr =
    const block_interfaces::msg::GripperAngles_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::GripperAngles_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::GripperAngles_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__block_interfaces__msg__GripperAngles
    std::shared_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__block_interfaces__msg__GripperAngles
    std::shared_ptr<block_interfaces::msg::GripperAngles_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const GripperAngles_ & other) const
  {
    if (this->header != other.header) {
      return false;
    }
    if (this->track_id != other.track_id) {
      return false;
    }
    if (this->image_width != other.image_width) {
      return false;
    }
    if (this->image_height != other.image_height) {
      return false;
    }
    if (this->bounds_valid != other.bounds_valid) {
      return false;
    }
    if (this->left_edge_px != other.left_edge_px) {
      return false;
    }
    if (this->right_edge_px != other.right_edge_px) {
      return false;
    }
    if (this->angles_ready != other.angles_ready) {
      return false;
    }
    if (this->left_angle_rad != other.left_angle_rad) {
      return false;
    }
    if (this->right_angle_rad != other.right_angle_rad) {
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
  bool operator!=(const GripperAngles_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct GripperAngles_

// alias to use template instance with default allocator
using GripperAngles =
  block_interfaces::msg::GripperAngles_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_HPP_
