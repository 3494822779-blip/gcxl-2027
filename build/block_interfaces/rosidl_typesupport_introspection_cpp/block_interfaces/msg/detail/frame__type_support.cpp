// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "block_interfaces/msg/detail/frame__struct.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

namespace block_interfaces
{

namespace msg
{

namespace rosidl_typesupport_introspection_cpp
{

void Frame_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) block_interfaces::msg::Frame(_init);
}

void Frame_fini_function(void * message_memory)
{
  auto typed_message = static_cast<block_interfaces::msg::Frame *>(message_memory);
  typed_message->~Frame();
}

size_t size_function__Frame__boxes(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<block_interfaces::msg::Box> *>(untyped_member);
  return member->size();
}

const void * get_const_function__Frame__boxes(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<block_interfaces::msg::Box> *>(untyped_member);
  return &member[index];
}

void * get_function__Frame__boxes(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<block_interfaces::msg::Box> *>(untyped_member);
  return &member[index];
}

void fetch_function__Frame__boxes(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const block_interfaces::msg::Box *>(
    get_const_function__Frame__boxes(untyped_member, index));
  auto & value = *reinterpret_cast<block_interfaces::msg::Box *>(untyped_value);
  value = item;
}

void assign_function__Frame__boxes(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<block_interfaces::msg::Box *>(
    get_function__Frame__boxes(untyped_member, index));
  const auto & value = *reinterpret_cast<const block_interfaces::msg::Box *>(untyped_value);
  item = value;
}

void resize_function__Frame__boxes(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<block_interfaces::msg::Box> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember Frame_message_member_array[2] = {
  {
    "image",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<sensor_msgs::msg::Image>(),  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(block_interfaces::msg::Frame, image),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "boxes",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<block_interfaces::msg::Box>(),  // members of sub message
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(block_interfaces::msg::Frame, boxes),  // bytes offset in struct
    nullptr,  // default value
    size_function__Frame__boxes,  // size() function pointer
    get_const_function__Frame__boxes,  // get_const(index) function pointer
    get_function__Frame__boxes,  // get(index) function pointer
    fetch_function__Frame__boxes,  // fetch(index, &value) function pointer
    assign_function__Frame__boxes,  // assign(index, value) function pointer
    resize_function__Frame__boxes  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers Frame_message_members = {
  "block_interfaces::msg",  // message namespace
  "Frame",  // message name
  2,  // number of fields
  sizeof(block_interfaces::msg::Frame),
  Frame_message_member_array,  // message members
  Frame_init_function,  // function to initialize message memory (memory has to be allocated)
  Frame_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t Frame_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &Frame_message_members,
  get_message_typesupport_handle_function,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace block_interfaces


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<block_interfaces::msg::Frame>()
{
  return &::block_interfaces::msg::rosidl_typesupport_introspection_cpp::Frame_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, block_interfaces, msg, Frame)() {
  return &::block_interfaces::msg::rosidl_typesupport_introspection_cpp::Frame_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
