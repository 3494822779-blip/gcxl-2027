// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_H_
#define BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_H_

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

/// Struct defined in msg/GripperAngles in the package block_interfaces.
/**
  * Perception proposal only, not a motor command. Angles are in radians.
  * Consumers MUST require angles_ready and reject expired acquisition timestamps.
 */
typedef struct block_interfaces__msg__GripperAngles
{
  std_msgs__msg__Header header;
  uint64_t track_id;
  uint32_t image_width;
  uint32_t image_height;
  bool bounds_valid;
  double left_edge_px;
  double right_edge_px;
  bool angles_ready;
  /// NaN until the calculation method is implemented; zero is NOT a fallback angle.
  double left_angle_rad;
  double right_angle_rad;
  double valid_for_sec;
  rosidl_runtime_c__String reason;
} block_interfaces__msg__GripperAngles;

// Struct for a sequence of block_interfaces__msg__GripperAngles.
typedef struct block_interfaces__msg__GripperAngles__Sequence
{
  block_interfaces__msg__GripperAngles * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} block_interfaces__msg__GripperAngles__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__STRUCT_H_
