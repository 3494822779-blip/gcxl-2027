// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from block_interfaces:msg/Box.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__BOX__BUILDER_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__BOX__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "block_interfaces/msg/detail/box__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace block_interfaces
{

namespace msg
{

namespace builder
{

class Init_Box_observed
{
public:
  explicit Init_Box_observed(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  ::block_interfaces::msg::Box observed(::block_interfaces::msg::Box::_observed_type arg)
  {
    msg_.observed = std::move(arg);
    return std::move(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_height
{
public:
  explicit Init_Box_height(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_observed height(::block_interfaces::msg::Box::_height_type arg)
  {
    msg_.height = std::move(arg);
    return Init_Box_observed(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_width
{
public:
  explicit Init_Box_width(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_height width(::block_interfaces::msg::Box::_width_type arg)
  {
    msg_.width = std::move(arg);
    return Init_Box_height(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_y
{
public:
  explicit Init_Box_y(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_width y(::block_interfaces::msg::Box::_y_type arg)
  {
    msg_.y = std::move(arg);
    return Init_Box_width(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_x
{
public:
  explicit Init_Box_x(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_y x(::block_interfaces::msg::Box::_x_type arg)
  {
    msg_.x = std::move(arg);
    return Init_Box_y(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_confidence
{
public:
  explicit Init_Box_confidence(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_x confidence(::block_interfaces::msg::Box::_confidence_type arg)
  {
    msg_.confidence = std::move(arg);
    return Init_Box_x(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_class_id
{
public:
  explicit Init_Box_class_id(::block_interfaces::msg::Box & msg)
  : msg_(msg)
  {}
  Init_Box_confidence class_id(::block_interfaces::msg::Box::_class_id_type arg)
  {
    msg_.class_id = std::move(arg);
    return Init_Box_confidence(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

class Init_Box_track_id
{
public:
  Init_Box_track_id()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Box_class_id track_id(::block_interfaces::msg::Box::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return Init_Box_class_id(msg_);
  }

private:
  ::block_interfaces::msg::Box msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::block_interfaces::msg::Box>()
{
  return block_interfaces::msg::builder::Init_Box_track_id();
}

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__BOX__BUILDER_HPP_
