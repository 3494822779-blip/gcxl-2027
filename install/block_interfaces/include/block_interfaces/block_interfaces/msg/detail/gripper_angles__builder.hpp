// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__BUILDER_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "block_interfaces/msg/detail/gripper_angles__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace block_interfaces
{

namespace msg
{

namespace builder
{

class Init_GripperAngles_reason
{
public:
  explicit Init_GripperAngles_reason(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  ::block_interfaces::msg::GripperAngles reason(::block_interfaces::msg::GripperAngles::_reason_type arg)
  {
    msg_.reason = std::move(arg);
    return std::move(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_valid_for_sec
{
public:
  explicit Init_GripperAngles_valid_for_sec(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_reason valid_for_sec(::block_interfaces::msg::GripperAngles::_valid_for_sec_type arg)
  {
    msg_.valid_for_sec = std::move(arg);
    return Init_GripperAngles_reason(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_right_angle_rad
{
public:
  explicit Init_GripperAngles_right_angle_rad(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_valid_for_sec right_angle_rad(::block_interfaces::msg::GripperAngles::_right_angle_rad_type arg)
  {
    msg_.right_angle_rad = std::move(arg);
    return Init_GripperAngles_valid_for_sec(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_left_angle_rad
{
public:
  explicit Init_GripperAngles_left_angle_rad(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_right_angle_rad left_angle_rad(::block_interfaces::msg::GripperAngles::_left_angle_rad_type arg)
  {
    msg_.left_angle_rad = std::move(arg);
    return Init_GripperAngles_right_angle_rad(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_angles_ready
{
public:
  explicit Init_GripperAngles_angles_ready(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_left_angle_rad angles_ready(::block_interfaces::msg::GripperAngles::_angles_ready_type arg)
  {
    msg_.angles_ready = std::move(arg);
    return Init_GripperAngles_left_angle_rad(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_right_edge_px
{
public:
  explicit Init_GripperAngles_right_edge_px(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_angles_ready right_edge_px(::block_interfaces::msg::GripperAngles::_right_edge_px_type arg)
  {
    msg_.right_edge_px = std::move(arg);
    return Init_GripperAngles_angles_ready(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_left_edge_px
{
public:
  explicit Init_GripperAngles_left_edge_px(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_right_edge_px left_edge_px(::block_interfaces::msg::GripperAngles::_left_edge_px_type arg)
  {
    msg_.left_edge_px = std::move(arg);
    return Init_GripperAngles_right_edge_px(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_bounds_valid
{
public:
  explicit Init_GripperAngles_bounds_valid(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_left_edge_px bounds_valid(::block_interfaces::msg::GripperAngles::_bounds_valid_type arg)
  {
    msg_.bounds_valid = std::move(arg);
    return Init_GripperAngles_left_edge_px(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_image_height
{
public:
  explicit Init_GripperAngles_image_height(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_bounds_valid image_height(::block_interfaces::msg::GripperAngles::_image_height_type arg)
  {
    msg_.image_height = std::move(arg);
    return Init_GripperAngles_bounds_valid(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_image_width
{
public:
  explicit Init_GripperAngles_image_width(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_image_height image_width(::block_interfaces::msg::GripperAngles::_image_width_type arg)
  {
    msg_.image_width = std::move(arg);
    return Init_GripperAngles_image_height(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_track_id
{
public:
  explicit Init_GripperAngles_track_id(::block_interfaces::msg::GripperAngles & msg)
  : msg_(msg)
  {}
  Init_GripperAngles_image_width track_id(::block_interfaces::msg::GripperAngles::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return Init_GripperAngles_image_width(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

class Init_GripperAngles_header
{
public:
  Init_GripperAngles_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_GripperAngles_track_id header(::block_interfaces::msg::GripperAngles::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_GripperAngles_track_id(msg_);
  }

private:
  ::block_interfaces::msg::GripperAngles msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::block_interfaces::msg::GripperAngles>()
{
  return block_interfaces::msg::builder::Init_GripperAngles_header();
}

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__BUILDER_HPP_
