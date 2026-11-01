// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from block_interfaces:msg/Target.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_H_
#define BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.h"
// Member 'last_observed'
#include "builtin_interfaces/msg/detail/time__struct.h"

/// Struct defined in msg/Target in the package block_interfaces.
typedef struct block_interfaces__msg__Target
{
  std_msgs__msg__Header header;
  uint32_t image_width;
  uint32_t image_height;
  bool locked;
  bool observed;
  bool valid;
  uint64_t track_id;
  int32_t class_id;
  double raw_x;
  double raw_y;
  double x;
  double y;
  builtin_interfaces__msg__Time last_observed;
} block_interfaces__msg__Target;

// Struct for a sequence of block_interfaces__msg__Target.
typedef struct block_interfaces__msg__Target__Sequence
{
  block_interfaces__msg__Target * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} block_interfaces__msg__Target__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__TARGET__STRUCT_H_
