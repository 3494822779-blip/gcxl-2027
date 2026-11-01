// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from block_interfaces:msg/Box.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__BOX__TRAITS_HPP_
#define BLOCK_INTERFACES__MSG__DETAIL__BOX__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "block_interfaces/msg/detail/box__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace block_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const Box & msg,
  std::ostream & out)
{
  out << "{";
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

  // member: confidence
  {
    out << "confidence: ";
    rosidl_generator_traits::value_to_yaml(msg.confidence, out);
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

  // member: width
  {
    out << "width: ";
    rosidl_generator_traits::value_to_yaml(msg.width, out);
    out << ", ";
  }

  // member: height
  {
    out << "height: ";
    rosidl_generator_traits::value_to_yaml(msg.height, out);
    out << ", ";
  }

  // member: observed
  {
    out << "observed: ";
    rosidl_generator_traits::value_to_yaml(msg.observed, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const Box & msg,
  std::ostream & out, size_t indentation = 0)
{
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

  // member: confidence
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "confidence: ";
    rosidl_generator_traits::value_to_yaml(msg.confidence, out);
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

  // member: width
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "width: ";
    rosidl_generator_traits::value_to_yaml(msg.width, out);
    out << "\n";
  }

  // member: height
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "height: ";
    rosidl_generator_traits::value_to_yaml(msg.height, out);
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
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const Box & msg, bool use_flow_style = false)
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
  const block_interfaces::msg::Box & msg,
  std::ostream & out, size_t indentation = 0)
{
  block_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use block_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const block_interfaces::msg::Box & msg)
{
  return block_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<block_interfaces::msg::Box>()
{
  return "block_interfaces::msg::Box";
}

template<>
inline const char * name<block_interfaces::msg::Box>()
{
  return "block_interfaces/msg/Box";
}

template<>
struct has_fixed_size<block_interfaces::msg::Box>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<block_interfaces::msg::Box>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<block_interfaces::msg::Box>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // BLOCK_INTERFACES__MSG__DETAIL__BOX__TRAITS_HPP_
