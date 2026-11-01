// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__TRAITS_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "block_interfaces/msg/detail/gripper_angles__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"

namespace block_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const GripperAngles & msg,
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

  // member: image_width
  {
    out << "image_width: ";
    rosidl_generator_traits::value_to_yaml(msg.image_width, out);
    out << ", ";
  }

  // member: image_height
  {
    out << "image_height: ";
    rosidl_generator_traits::value_to_yaml(msg.image_height, out);
    out << ", ";
  }

  // member: bounds_valid
  {
    out << "bounds_valid: ";
    rosidl_generator_traits::value_to_yaml(msg.bounds_valid, out);
    out << ", ";
  }

  // member: left_edge_px
  {
    out << "left_edge_px: ";
    rosidl_generator_traits::value_to_yaml(msg.left_edge_px, out);
    out << ", ";
  }

  // member: right_edge_px
  {
    out << "right_edge_px: ";
    rosidl_generator_traits::value_to_yaml(msg.right_edge_px, out);
    out << ", ";
  }

  // member: angles_ready
  {
    out << "angles_ready: ";
    rosidl_generator_traits::value_to_yaml(msg.angles_ready, out);
    out << ", ";
  }

  // member: left_angle_rad
  {
    out << "left_angle_rad: ";
    rosidl_generator_traits::value_to_yaml(msg.left_angle_rad, out);
    out << ", ";
  }

  // member: right_angle_rad
  {
    out << "right_angle_rad: ";
    rosidl_generator_traits::value_to_yaml(msg.right_angle_rad, out);
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
  const GripperAngles & msg,
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

  // member: image_width
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "image_width: ";
    rosidl_generator_traits::value_to_yaml(msg.image_width, out);
    out << "\n";
  }

  // member: image_height
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "image_height: ";
    rosidl_generator_traits::value_to_yaml(msg.image_height, out);
    out << "\n";
  }

  // member: bounds_valid
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "bounds_valid: ";
    rosidl_generator_traits::value_to_yaml(msg.bounds_valid, out);
    out << "\n";
  }

  // member: left_edge_px
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "left_edge_px: ";
    rosidl_generator_traits::value_to_yaml(msg.left_edge_px, out);
    out << "\n";
  }

  // member: right_edge_px
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "right_edge_px: ";
    rosidl_generator_traits::value_to_yaml(msg.right_edge_px, out);
    out << "\n";
  }

  // member: angles_ready
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "angles_ready: ";
    rosidl_generator_traits::value_to_yaml(msg.angles_ready, out);
    out << "\n";
  }

  // member: left_angle_rad
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "left_angle_rad: ";
    rosidl_generator_traits::value_to_yaml(msg.left_angle_rad, out);
    out << "\n";
  }

  // member: right_angle_rad
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "right_angle_rad: ";
    rosidl_generator_traits::value_to_yaml(msg.right_angle_rad, out);
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

inline std::string to_yaml(const GripperAngles & msg, bool use_flow_style = false)
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
  const block_interfaces::msg::GripperAngles & msg,
  std::ostream & out, size_t indentation = 0)
{
  block_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use block_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const block_interfaces::msg::GripperAngles & msg)
{
  return block_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<block_interfaces::msg::GripperAngles>()
{
  return "block_interfaces::msg::GripperAngles";
}

template<>
inline const char * name<block_interfaces::msg::GripperAngles>()
{
  return "block_interfaces/msg/GripperAngles";
}

template<>
struct has_fixed_size<block_interfaces::msg::GripperAngles>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<block_interfaces::msg::GripperAngles>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<block_interfaces::msg::GripperAngles>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__TRAITS_HPP_
