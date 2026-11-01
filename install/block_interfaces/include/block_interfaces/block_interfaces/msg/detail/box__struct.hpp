// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from block_interfaces:msg/Box.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__block_interfaces__msg__Box __attribute__((deprecated))
#else
# define DEPRECATED__block_interfaces__msg__Box __declspec(deprecated)
#endif

namespace block_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct Box_
{
  using Type = Box_<ContainerAllocator>;

  explicit Box_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->class_id = 0l;
      this->confidence = 0.0f;
      this->x = 0.0f;
      this->y = 0.0f;
      this->width = 0.0f;
      this->height = 0.0f;
      this->observed = false;
    }
  }

  explicit Box_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->track_id = 0ull;
      this->class_id = 0l;
      this->confidence = 0.0f;
      this->x = 0.0f;
      this->y = 0.0f;
      this->width = 0.0f;
      this->height = 0.0f;
      this->observed = false;
    }
  }

  // field types and members
  using _track_id_type =
    uint64_t;
  _track_id_type track_id;
  using _class_id_type =
    int32_t;
  _class_id_type class_id;
  using _confidence_type =
    float;
  _confidence_type confidence;
  using _x_type =
    float;
  _x_type x;
  using _y_type =
    float;
  _y_type y;
  using _width_type =
    float;
  _width_type width;
  using _height_type =
    float;
  _height_type height;
  using _observed_type =
    bool;
  _observed_type observed;

  // setters for named parameter idiom
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
  Type & set__confidence(
    const float & _arg)
  {
    this->confidence = _arg;
    return *this;
  }
  Type & set__x(
    const float & _arg)
  {
    this->x = _arg;
    return *this;
  }
  Type & set__y(
    const float & _arg)
  {
    this->y = _arg;
    return *this;
  }
  Type & set__width(
    const float & _arg)
  {
    this->width = _arg;
    return *this;
  }
  Type & set__height(
    const float & _arg)
  {
    this->height = _arg;
    return *this;
  }
  Type & set__observed(
    const bool & _arg)
  {
    this->observed = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    block_interfaces::msg::Box_<ContainerAllocator> *;
  using ConstRawPtr =
    const block_interfaces::msg::Box_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<block_interfaces::msg::Box_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<block_interfaces::msg::Box_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Box_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Box_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Box_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Box_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<block_interfaces::msg::Box_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<block_interfaces::msg::Box_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__block_interfaces__msg__Box
    std::shared_ptr<block_interfaces::msg::Box_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__block_interfaces__msg__Box
    std::shared_ptr<block_interfaces::msg::Box_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const Box_ & other) const
  {
    if (this->track_id != other.track_id) {
      return false;
    }
    if (this->class_id != other.class_id) {
      return false;
    }
    if (this->confidence != other.confidence) {
      return false;
    }
    if (this->x != other.x) {
      return false;
    }
    if (this->y != other.y) {
      return false;
    }
    if (this->width != other.width) {
      return false;
    }
    if (this->height != other.height) {
      return false;
    }
    if (this->observed != other.observed) {
      return false;
    }
    return true;
  }
  bool operator!=(const Box_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct Box_

// alias to use template instance with default allocator
using Box =
  block_interfaces::msg::Box_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_HPP_
