// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "block_interfaces/msg/detail/frame__rosidl_typesupport_introspection_c.h"
#include "block_interfaces/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "block_interfaces/msg/detail/frame__functions.h"
#include "block_interfaces/msg/detail/frame__struct.h"


// Include directives for member types
// Member `image`
#include "sensor_msgs/msg/image.h"
// Member `image`
#include "sensor_msgs/msg/detail/image__rosidl_typesupport_introspection_c.h"
// Member `boxes`
#include "block_interfaces/msg/box.h"
// Member `boxes`
#include "block_interfaces/msg/detail/box__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  block_interfaces__msg__Frame__init(message_memory);
}

void block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_fini_function(void * message_memory)
{
  block_interfaces__msg__Frame__fini(message_memory);
}

size_t block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__size_function__Frame__boxes(
  const void * untyped_member)
{
  const block_interfaces__msg__Box__Sequence * member =
    (const block_interfaces__msg__Box__Sequence *)(untyped_member);
  return member->size;
}

const void * block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_const_function__Frame__boxes(
  const void * untyped_member, size_t index)
{
  const block_interfaces__msg__Box__Sequence * member =
    (const block_interfaces__msg__Box__Sequence *)(untyped_member);
  return &member->data[index];
}

void * block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_function__Frame__boxes(
  void * untyped_member, size_t index)
{
  block_interfaces__msg__Box__Sequence * member =
    (block_interfaces__msg__Box__Sequence *)(untyped_member);
  return &member->data[index];
}

void block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__fetch_function__Frame__boxes(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const block_interfaces__msg__Box * item =
    ((const block_interfaces__msg__Box *)
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_const_function__Frame__boxes(untyped_member, index));
  block_interfaces__msg__Box * value =
    (block_interfaces__msg__Box *)(untyped_value);
  *value = *item;
}

void block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__assign_function__Frame__boxes(
  void * untyped_member, size_t index, const void * untyped_value)
{
  block_interfaces__msg__Box * item =
    ((block_interfaces__msg__Box *)
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_function__Frame__boxes(untyped_member, index));
  const block_interfaces__msg__Box * value =
    (const block_interfaces__msg__Box *)(untyped_value);
  *item = *value;
}

bool block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__resize_function__Frame__boxes(
  void * untyped_member, size_t size)
{
  block_interfaces__msg__Box__Sequence * member =
    (block_interfaces__msg__Box__Sequence *)(untyped_member);
  block_interfaces__msg__Box__Sequence__fini(member);
  return block_interfaces__msg__Box__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_member_array[2] = {
  {
    "image",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(block_interfaces__msg__Frame, image),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "boxes",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(block_interfaces__msg__Frame, boxes),  // bytes offset in struct
    NULL,  // default value
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__size_function__Frame__boxes,  // size() function pointer
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_const_function__Frame__boxes,  // get_const(index) function pointer
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__get_function__Frame__boxes,  // get(index) function pointer
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__fetch_function__Frame__boxes,  // fetch(index, &value) function pointer
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__assign_function__Frame__boxes,  // assign(index, value) function pointer
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__resize_function__Frame__boxes  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_members = {
  "block_interfaces__msg",  // message namespace
  "Frame",  // message name
  2,  // number of fields
  sizeof(block_interfaces__msg__Frame),
  block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_member_array,  // message members
  block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_init_function,  // function to initialize message memory (memory has to be allocated)
  block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_type_support_handle = {
  0,
  &block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_block_interfaces
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, block_interfaces, msg, Frame)() {
  block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_member_array[0].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, sensor_msgs, msg, Image)();
  block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_member_array[1].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, block_interfaces, msg, Box)();
  if (!block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_type_support_handle.typesupport_identifier) {
    block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &block_interfaces__msg__Frame__rosidl_typesupport_introspection_c__Frame_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
