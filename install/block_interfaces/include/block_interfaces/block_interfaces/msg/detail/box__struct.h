// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from block_interfaces:msg/Box.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_H_
#define BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/Box in the package block_interfaces.
typedef struct block_interfaces__msg__Box
{
  uint64_t track_id;
  int32_t class_id;
  float confidence;
  float x;
  float y;
  float width;
  float height;
  bool observed;
} block_interfaces__msg__Box;

// Struct for a sequence of block_interfaces__msg__Box.
typedef struct block_interfaces__msg__Box__Sequence
{
  block_interfaces__msg__Box * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} block_interfaces__msg__Box__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__BOX__STRUCT_H_
