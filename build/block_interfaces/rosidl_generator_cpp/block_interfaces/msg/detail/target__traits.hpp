// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from block_interfaces:msg/Target.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__TARGET__TRAITS_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__TARGET__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "block_interfaces/msg/detail/target__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"
// Member 'last_observed'
#include "builtin_interfaces/msg/detail/time__traits.hpp"

namespace block_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const Target & msg,
  std::ostream & out)
{
  out << "{";
  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
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

  // member: locked
  {
    out << "locked: ";
    rosidl_generator_traits::value_to_yaml(msg.locked, out);
    out << ", ";
  }

  // member: observed
  {
    out << "observed: ";
    rosidl_generator_traits::value_to_yaml(msg.observed, out);
    out << ", ";
  }

  // member: valid
  {
    out << "valid: ";
    rosidl_generator_traits::value_to_yaml(msg.valid, out);
    out << ", ";
  }

  // member: track_id
  {
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << ", ";
  }

  // member: class_id
  {
    out << "class_id: ";
    rosidl_generator_traits::value_to_yaml(msg.class_id, out);
    out << ", ";
  }

  // member: raw_x
  {
    out << "raw_x: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_x, out);
    out << ", ";
  }

  // member: raw_y
  {
    out << "raw_y: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_y, out);
    out << ", ";
  }

  // member: x
  {
    out << "x: ";
    rosidl_generator_traits::value_to_yaml(msg.x, out);
    out << ", ";
  }

  // member: y
  {
    out << "y: ";
    rosidl_generator_traits::value_to_yaml(msg.y, out);
    out << ", ";
  }

  // member: last_observed
  {
    out << "last_observed: ";
    to_flow_style_yaml(msg.last_observed, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const Target & msg,
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

  // member: locked
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "locked: ";
    rosidl_generator_traits::value_to_yaml(msg.locked, out);
    out << "\n";
  }

  // member: observed
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "observed: ";
    rosidl_generator_traits::value_to_yaml(msg.observed, out);
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

  // member: track_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << "\n";
  }

  // member: class_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "class_id: ";
    rosidl_generator_traits::value_to_yaml(msg.class_id, out);
    out << "\n";
  }

  // member: raw_x
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "raw_x: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_x, out);
    out << "\n";
  }

  // member: raw_y
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "raw_y: ";
    rosidl_generator_traits::value_to_yaml(msg.raw_y, out);
    out << "\n";
  }

  // member: x
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "x: ";
    rosidl_generator_traits::value_to_yaml(msg.x, out);
    out << "\n";
  }

  // member: y
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "y: ";
    rosidl_generator_traits::value_to_yaml(msg.y, out);
    out << "\n";
  }

  // member: last_observed
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "last_observed:\n";
    to_block_style_yaml(msg.last_observed, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const Target & msg, bool use_flow_style = false)
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
  const block_interfaces::msg::Target & msg,
  std::ostream & out, size_t indentation = 0)
{
  block_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use block_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const block_interfaces::msg::Target & msg)
{
  return block_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<block_interfaces::msg::Target>()
{
  return "block_interfaces::msg::Target";
}

template<>
inline const char * name<block_interfaces::msg::Target>()
{
  return "block_interfaces/msg/Target";
}

template<>
struct has_fixed_size<block_interfaces::msg::Target>
  : std::integral_constant<bool, has_fixed_size<builtin_interfaces::msg::Time>::value && has_fixed_size<std_msgs::msg::Header>::value> {};

template<>
struct has_bounded_size<block_interfaces::msg::Target>
  : std::integral_constant<bool, has_bounded_size<builtin_interfaces::msg::Time>::value && has_bounded_size<std_msgs::msg::Header>::value> {};

template<>
struct is_message<block_interfaces::msg::Target>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // BLOCK_INTERFACES__MSG__DETAIL__TARGET__TRAITS_HPP_
