// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from block_interfaces:msg/Target.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__TARGET__BUILDER_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__TARGET__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "block_interfaces/msg/detail/target__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace block_interfaces
{

namespace msg
{

namespace builder
{

class Init_Target_last_observed
{
public:
  explicit Init_Target_last_observed(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  ::block_interfaces::msg::Target last_observed(::block_interfaces::msg::Target::_last_observed_type arg)
  {
    msg_.last_observed = std::move(arg);
    return std::move(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_y
{
public:
  explicit Init_Target_y(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_last_observed y(::block_interfaces::msg::Target::_y_type arg)
  {
    msg_.y = std::move(arg);
    return Init_Target_last_observed(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_x
{
public:
  explicit Init_Target_x(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_y x(::block_interfaces::msg::Target::_x_type arg)
  {
    msg_.x = std::move(arg);
    return Init_Target_y(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_raw_y
{
public:
  explicit Init_Target_raw_y(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_x raw_y(::block_interfaces::msg::Target::_raw_y_type arg)
  {
    msg_.raw_y = std::move(arg);
    return Init_Target_x(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_raw_x
{
public:
  explicit Init_Target_raw_x(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_raw_y raw_x(::block_interfaces::msg::Target::_raw_x_type arg)
  {
    msg_.raw_x = std::move(arg);
    return Init_Target_raw_y(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_class_id
{
public:
  explicit Init_Target_class_id(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_raw_x class_id(::block_interfaces::msg::Target::_class_id_type arg)
  {
    msg_.class_id = std::move(arg);
    return Init_Target_raw_x(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_track_id
{
public:
  explicit Init_Target_track_id(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_class_id track_id(::block_interfaces::msg::Target::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return Init_Target_class_id(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_valid
{
public:
  explicit Init_Target_valid(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_track_id valid(::block_interfaces::msg::Target::_valid_type arg)
  {
    msg_.valid = std::move(arg);
    return Init_Target_track_id(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_observed
{
public:
  explicit Init_Target_observed(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_valid observed(::block_interfaces::msg::Target::_observed_type arg)
  {
    msg_.observed = std::move(arg);
    return Init_Target_valid(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_locked
{
public:
  explicit Init_Target_locked(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_observed locked(::block_interfaces::msg::Target::_locked_type arg)
  {
    msg_.locked = std::move(arg);
    return Init_Target_observed(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_image_height
{
public:
  explicit Init_Target_image_height(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_locked image_height(::block_interfaces::msg::Target::_image_height_type arg)
  {
    msg_.image_height = std::move(arg);
    return Init_Target_locked(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_image_width
{
public:
  explicit Init_Target_image_width(::block_interfaces::msg::Target & msg)
  : msg_(msg)
  {}
  Init_Target_image_height image_width(::block_interfaces::msg::Target::_image_width_type arg)
  {
    msg_.image_width = std::move(arg);
    return Init_Target_image_height(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

class Init_Target_header
{
public:
  Init_Target_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Target_image_width header(::block_interfaces::msg::Target::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_Target_image_width(msg_);
  }

private:
  ::block_interfaces::msg::Target msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::block_interfaces::msg::Target>()
{
  return block_interfaces::msg::builder::Init_Target_header();
}

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__TARGET__BUILDER_HPP_
