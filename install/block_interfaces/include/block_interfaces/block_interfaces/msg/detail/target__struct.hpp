// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from block_interfaces:msg/Target.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_HPP_

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
// Member 'last_observed'
#include "builtin_interfaces/msg/detail/time__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__block_interfaces__msg__Target __attribute__((deprecated))
#else
# define DEPRECATED__block_interfaces__msg__Target __declspec(deprecated)
#endif

namespace block_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct Target_
{
  using Type = Target_<ContainerAllocator>;

  explicit Target_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init),
    last_observed(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->image_width = 0ul;
      this->image_height = 0ul;
      this->locked = false;
      this->observed = false;
      this->valid = false;
      this->track_id = 0ull;
      this->class_id = 0l;
      this->raw_x = 0.0;
      this->raw_y = 0.0;
      this->x = 0.0;
      this->y = 0.0;
    }
  }

  explicit Target_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_alloc, _init),
    last_observed(_alloc, _init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->image_width = 0ul;
      this->image_height = 0ul;
      this->locked = false;
      this->observed = false;
      this->valid = false;
      this->track_id = 0ull;
      this->class_id = 0l;
      this->raw_x = 0.0;
      this->raw_y = 0.0;
      this->x = 0.0;
      this->y = 0.0;
    }
  }

  // field types and members
  using _header_type =
    std_msgs::msg::Header_<ContainerAllocator>;
  _header_type header;
  using _image_width_type =
    uint32_t;
  _image_width_type image_width;
  using _image_height_type =
    uint32_t;
  _image_height_type image_height;
  using _locked_type =
    bool;
  _locked_type locked;
  using _observed_type =
    bool;
  _observed_type observed;
  using _valid_type =
    bool;
  _valid_type valid;
  using _track_id_type =
    uint64_t;
  _track_id_type track_id;
  using _class_id_type =
    int32_t;
  _class_id_type class_id;
  using _raw_x_type =
    double;
  _raw_x_type raw_x;
  using _raw_y_type =
    double;
  _raw_y_type raw_y;
  using _x_type =
    double;
  _x_type x;
  using _y_type =
    double;
  _y_type y;
  using _last_observed_type =
    builtin_interfaces::msg::Time_<ContainerAllocator>;
  _last_observed_type last_observed;

  // setters for named parameter idiom
  Type & set__header(
    const std_msgs::msg::Header_<ContainerAllocator> & _arg)
  {
    this->header = _arg;
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
  Type & set__locked(
    const bool & _arg)
  {
    this->locked = _arg;
    return *this;
  }
  Type & set__observed(
    const bool & _arg)
  {
    this->observed = _arg;
    return *this;
  }
  Type & set__valid(
    const bool & _arg)
  {
    this->valid = _arg;
    return *this;
  }
  Type & set__track_id(
    const uint64_t & _arg)
  {
    this->track_id = _arg;
    return *this;
  }
  Type & set__class_id(
    const int32_t & _arg)
  {
    this->class_id = _arg;
    return *this;
  }
  Type & set__raw_x(
    const double & _arg)
  {
    this->raw_x = _arg;
    return *this;
  }
  Type & set__raw_y(
    const double & _arg)
  {
    this->raw_y = _arg;
    return *this;
  }
  Type & set__x(
    const double & _arg)
  {
    this->x = _arg;
    return *this;
  }
  Type & set__y(
    const double & _arg)
  {
    this->y = _arg;
    return *this;
  }
  Type & set__last_observed(
    const builtin_interfaces::msg::Time_<ContainerAllocator> & _arg)
  {
    this->last_observed = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    block_interfaces::msg::Target_<ContainerAllocator> *;
  using ConstRawPtr =
    const block_interfaces::msg::Target_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<block_interfaces::msg::Target_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<block_interfaces::msg::Target_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Target_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Target_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Target_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Target_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<block_interfaces::msg::Target_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<block_interfaces::msg::Target_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__block_interfaces__msg__Target
    std::shared_ptr<block_interfaces::msg::Target_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__block_interfaces__msg__Target
    std::shared_ptr<block_interfaces::msg::Target_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const Target_ & other) const
  {
    if (this->header != other.header) {
      return false;
    }
    if (this->image_width != other.image_width) {
      return false;
    }
    if (this->image_height != other.image_height) {
      return false;
    }
    if (this->locked != other.locked) {
      return false;
    }
    if (this->observed != other.observed) {
      return false;
    }
    if (this->valid != other.valid) {
      return false;
    }
    if (this->track_id != other.track_id) {
      return false;
    }
    if (this->class_id != other.class_id) {
      return false;
    }
    if (this->raw_x != other.raw_x) {
      return false;
    }
    if (this->raw_y != other.raw_y) {
      return false;
    }
    if (this->x != other.x) {
      return false;
    }
    if (this->y != other.y) {
      return false;
    }
    if (this->last_observed != other.last_observed) {
      return false;
    }
    return true;
  }
  bool operator!=(const Target_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct Target_

// alias to use template instance with default allocator
using Target =
  block_interfaces::msg::Target_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_HPP_
