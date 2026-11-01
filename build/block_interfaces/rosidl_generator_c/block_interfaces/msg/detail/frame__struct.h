// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_H_
#define BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'image'
#include "sensor_msgs/msg/detail/image__struct.h"
// Member 'boxes'
#include "block_interfaces/msg/detail/box__struct.h"

/// Struct defined in msg/Frame in the package block_interfaces.
/**
  * Image and boxes share exactly the same acquisition timestamp and pixel coordinates.
 */
typedef struct block_interfaces__msg__Frame
{
  sensor_msgs__msg__Image image;
  block_interfaces__msg__Box__Sequence boxes;
} block_interfaces__msg__Frame;

// Struct for a sequence of block_interfaces__msg__Frame.
typedef struct block_interfaces__msg__Frame__Sequence
{
  block_interfaces__msg__Frame * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} block_interfaces__msg__Frame__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__FRAME__STRUCT_H_
