// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'image'
#include "sensor_msgs/msg/detail/image__struct.hpp"
// Member 'boxes'
#include "block_interfaces/msg/detail/box__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__block_interfaces__msg__Frame __attribute__((deprecated))
#else
# define DEPRECATED__block_interfaces__msg__Frame __declspec(deprecated)
#endif

namespace block_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct Frame_
{
  using Type = Frame_<ContainerAllocator>;

  explicit Frame_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : image(_init)
  {
    (void)_init;
  }

  explicit Frame_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : image(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _image_type =
    sensor_msgs::msg::Image_<ContainerAllocator>;
  _image_type image;
  using _boxes_type =
    std::vector<block_interfaces::msg::Box_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<block_interfaces::msg::Box_<ContainerAllocator>>>;
  _boxes_type boxes;

  // setters for named parameter idiom
  Type & set__image(
    const sensor_msgs::msg::Image_<ContainerAllocator> & _arg)
  {
    this->image = _arg;
    return *this;
  }
  Type & set__boxes(
    const std::vector<block_interfaces::msg::Box_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<block_interfaces::msg::Box_<ContainerAllocator>>> & _arg)
  {
    this->boxes = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    block_interfaces::msg::Frame_<ContainerAllocator> *;
  using ConstRawPtr =
    const block_interfaces::msg::Frame_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<block_interfaces::msg::Frame_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<block_interfaces::msg::Frame_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Frame_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Frame_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      block_interfaces::msg::Frame_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<block_interfaces::msg::Frame_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<block_interfaces::msg::Frame_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<block_interfaces::msg::Frame_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__block_interfaces__msg__Frame
    std::shared_ptr<block_interfaces::msg::Frame_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__block_interfaces__msg__Frame
    std::shared_ptr<block_interfaces::msg::Frame_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const Frame_ & other) const
  {
    if (this->image != other.image) {
      return false;
    }
    if (this->boxes != other.boxes) {
      return false;
    }
    return true;
  }
  bool operator!=(const Frame_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct Frame_

// alias to use template instance with default allocator
using Frame =
  block_interfaces::msg::Frame_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_HPP_
