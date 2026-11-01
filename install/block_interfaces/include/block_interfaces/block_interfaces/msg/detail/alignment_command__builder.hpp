// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__BUILDER_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "block_interfaces/msg/detail/alignment_command__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace block_interfaces
{

namespace msg
{

namespace builder
{

class Init_AlignmentCommand_reason
{
public:
  explicit Init_AlignmentCommand_reason(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  ::block_interfaces::msg::AlignmentCommand reason(::block_interfaces::msg::AlignmentCommand::_reason_type arg)
  {
    msg_.reason = std::move(arg);
    return std::move(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_valid_for_sec
{
public:
  explicit Init_AlignmentCommand_valid_for_sec(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_reason valid_for_sec(::block_interfaces::msg::AlignmentCommand::_valid_for_sec_type arg)
  {
    msg_.valid_for_sec = std::move(arg);
    return Init_AlignmentCommand_reason(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_grasp_request
{
public:
  explicit Init_AlignmentCommand_grasp_request(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_valid_for_sec grasp_request(::block_interfaces::msg::AlignmentCommand::_grasp_request_type arg)
  {
    msg_.grasp_request = std::move(arg);
    return Init_AlignmentCommand_valid_for_sec(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_aligned
{
public:
  explicit Init_AlignmentCommand_aligned(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_grasp_request aligned(::block_interfaces::msg::AlignmentCommand::_aligned_type arg)
  {
    msg_.aligned = std::move(arg);
    return Init_AlignmentCommand_grasp_request(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_pitch_rate
{
public:
  explicit Init_AlignmentCommand_pitch_rate(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_aligned pitch_rate(::block_interfaces::msg::AlignmentCommand::_pitch_rate_type arg)
  {
    msg_.pitch_rate = std::move(arg);
    return Init_AlignmentCommand_aligned(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_yaw_rate
{
public:
  explicit Init_AlignmentCommand_yaw_rate(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_pitch_rate yaw_rate(::block_interfaces::msg::AlignmentCommand::_yaw_rate_type arg)
  {
    msg_.yaw_rate = std::move(arg);
    return Init_AlignmentCommand_pitch_rate(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_error_y
{
public:
  explicit Init_AlignmentCommand_error_y(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_yaw_rate error_y(::block_interfaces::msg::AlignmentCommand::_error_y_type arg)
  {
    msg_.error_y = std::move(arg);
    return Init_AlignmentCommand_yaw_rate(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_error_x
{
public:
  explicit Init_AlignmentCommand_error_x(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_error_y error_x(::block_interfaces::msg::AlignmentCommand::_error_x_type arg)
  {
    msg_.error_x = std::move(arg);
    return Init_AlignmentCommand_error_y(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_predicted
{
public:
  explicit Init_AlignmentCommand_predicted(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_error_x predicted(::block_interfaces::msg::AlignmentCommand::_predicted_type arg)
  {
    msg_.predicted = std::move(arg);
    return Init_AlignmentCommand_error_x(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_valid
{
public:
  explicit Init_AlignmentCommand_valid(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_predicted valid(::block_interfaces::msg::AlignmentCommand::_valid_type arg)
  {
    msg_.valid = std::move(arg);
    return Init_AlignmentCommand_predicted(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_track_id
{
public:
  explicit Init_AlignmentCommand_track_id(::block_interfaces::msg::AlignmentCommand & msg)
  : msg_(msg)
  {}
  Init_AlignmentCommand_valid track_id(::block_interfaces::msg::AlignmentCommand::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return Init_AlignmentCommand_valid(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

class Init_AlignmentCommand_header
{
public:
  Init_AlignmentCommand_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_AlignmentCommand_track_id header(::block_interfaces::msg::AlignmentCommand::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_AlignmentCommand_track_id(msg_);
  }

private:
  ::block_interfaces::msg::AlignmentCommand msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::block_interfaces::msg::AlignmentCommand>()
{
  return block_interfaces::msg::builder::Init_AlignmentCommand_header();
}

}  // namespace block_interfaces

#endif  // BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__BUILDER_HPP_
