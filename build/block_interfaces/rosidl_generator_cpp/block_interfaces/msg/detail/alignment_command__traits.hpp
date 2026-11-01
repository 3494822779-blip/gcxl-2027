// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__TRAITS_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "block_interfaces/msg/detail/alignment_command__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"

namespace block_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const AlignmentCommand & msg,
  std::ostream & out)
{
  out << "{";
  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
    out << ", ";
  }

  // member: track_id
  {
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << ", ";
  }

  // member: valid
  {
    out << "valid: ";
    rosidl_generator_traits::value_to_yaml(msg.valid, out);
    out << ", ";
  }

  // member: predicted
  {
    out << "predicted: ";
    rosidl_generator_traits::value_to_yaml(msg.predicted, out);
    out << ", ";
  }

  // member: error_x
  {
    out << "error_x: ";
    rosidl_generator_traits::value_to_yaml(msg.error_x, out);
    out << ", ";
  }

  // member: error_y
  {
    out << "error_y: ";
    rosidl_generator_traits::value_to_yaml(msg.error_y, out);
    out << ", ";
  }

  // member: yaw_rate
  {
    out << "yaw_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.yaw_rate, out);
    out << ", ";
  }

  // member: pitch_rate
  {
    out << "pitch_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.pitch_rate, out);
    out << ", ";
  }

  // member: aligned
  {
    out << "aligned: ";
    rosidl_generator_traits::value_to_yaml(msg.aligned, out);
    out << ", ";
  }

  // member: grasp_request
  {
    out << "grasp_request: ";
    rosidl_generator_traits::value_to_yaml(msg.grasp_request, out);
    out << ", ";
  }

  // member: valid_for_sec
  {
    out << "valid_for_sec: ";
    rosidl_generator_traits::value_to_yaml(msg.valid_for_sec, out);
    out << ", ";
  }

  // member: reason
  {
    out << "reason: ";
    rosidl_generator_traits::value_to_yaml(msg.reason, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const AlignmentCommand & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: header
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "header:\n";
    to_block_style_yaml(msg.header, out, indentation + 2);
  }

  // member: track_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << "\n";
  }

  // member: valid
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "valid: ";
    rosidl_generator_traits::value_to_yaml(msg.valid, out);
    out << "\n";
  }

  // member: predicted
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "predicted: ";
    rosidl_generator_traits::value_to_yaml(msg.predicted, out);
    out << "\n";
  }

  // member: error_x
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "error_x: ";
    rosidl_generator_traits::value_to_yaml(msg.error_x, out);
    out << "\n";
  }

  // member: error_y
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "error_y: ";
    rosidl_generator_traits::value_to_yaml(msg.error_y, out);
    out << "\n";
  }

  // member: yaw_rate
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "yaw_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.yaw_rate, out);
    out << "\n";
  }

  // member: pitch_rate
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "pitch_rate: ";
    rosidl_generator_traits::value_to_yaml(msg.pitch_rate, out);
    out << "\n";
  }

  // member: aligned
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "aligned: ";
    rosidl_generator_traits::value_to_yaml(msg.aligned, out);
    out << "\n";
  }

  // member: grasp_request
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "grasp_request: ";
    rosidl_generator_traits::value_to_yaml(msg.grasp_request, out);
    out << "\n";
  }

  // member: valid_for_sec
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "valid_for_sec: ";
    rosidl_generator_traits::value_to_yaml(msg.valid_for_sec, out);
    out << "\n";
  }

  // member: reason
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "reason: ";
    rosidl_generator_traits::value_to_yaml(msg.reason, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const AlignmentCommand & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace block_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use block_interfaces::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const block_interfaces::msg::AlignmentCommand & msg,
  std::ostream & out, size_t indentation = 0)
{
  block_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use block_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const block_interfaces::msg::AlignmentCommand & msg)
{
  return block_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<block_interfaces::msg::AlignmentCommand>()
{
  return "block_interfaces::msg::AlignmentCommand";
}

template<>
inline const char * name<block_interfaces::msg::AlignmentCommand>()
{
  return "block_interfaces/msg/AlignmentCommand";
}

template<>
struct has_fixed_size<block_interfaces::msg::AlignmentCommand>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<block_interfaces::msg::AlignmentCommand>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<block_interfaces::msg::AlignmentCommand>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__TRAITS_HPP_
