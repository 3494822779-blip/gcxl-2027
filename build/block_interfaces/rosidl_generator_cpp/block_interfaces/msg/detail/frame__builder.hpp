// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__FRAME__BUILDER_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__FRAME__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "block_interfaces/msg/detail/frame__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace block_interfaces
{

namespace msg
{

namespace builder
{

class Init_Frame_boxes
{
public:
  explicit Init_Frame_boxes(::block_interfaces::msg::Frame & msg)
  : msg_(msg)
  {}
  ::block_interfaces::msg::Frame boxes(::block_interfaces::msg::Frame::_boxes_type arg)
  {
    msg_.boxes = std::move(arg);
    return std::move(msg_);
  }

private:
  ::block_interfaces::msg::Frame msg_;
};

class Init_Frame_image
{
public:
  Init_Frame_image()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Frame_boxes image(::block_interfaces::msg::Frame::_image_type arg)
  {
    msg_.image = std::move(arg);
    return Init_Frame_boxes(msg_);
  }

private:
  ::block_interfaces::msg::Frame msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::block_interfaces::msg::Frame>()
{
  return block_interfaces::msg::builder::Init_Frame_image();
}

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__FRAME__BUILDER_HPP_
