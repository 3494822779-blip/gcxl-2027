// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_H_
#define BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_H_

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
// Member 'reason'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/AlignmentCommand in the package block_interfaces.
/**
  * Errors are normalized by half image size. Rates are rad/s; receiver must enforce TTL.
 */
typedef struct block_interfaces__msg__AlignmentCommand
{
  std_msgs__msg__Header header;
  uint64_t track_id;
  bool valid;
  bool predicted;
  double error_x;
  double error_y;
  double yaw_rate;
  double pitch_rate;
  bool aligned;
  /// One-shot request per lock. External executor acknowledges by resetting the lock.
  bool grasp_request;
  double valid_for_sec;
  rosidl_runtime_c__String reason;
} block_interfaces__msg__AlignmentCommand;

// Struct for a sequence of block_interfaces__msg__AlignmentCommand.
typedef struct block_interfaces__msg__AlignmentCommand__Sequence
{
  block_interfaces__msg__AlignmentCommand * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} block_interfaces__msg__AlignmentCommand__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__STRUCT_H_
