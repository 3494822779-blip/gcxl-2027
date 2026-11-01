// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__FUNCTIONS_H_
#define BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "block_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "block_interfaces/msg/detail/gripper_angles__struct.h"

/// Initialize msg/GripperAngles message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * block_interfaces__msg__GripperAngles
 * )) before or use
 * block_interfaces__msg__GripperAngles__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__init(block_interfaces__msg__GripperAngles * msg);

/// Finalize msg/GripperAngles message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__GripperAngles__fini(block_interfaces__msg__GripperAngles * msg);

/// Create msg/GripperAngles message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * block_interfaces__msg__GripperAngles__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__GripperAngles *
block_interfaces__msg__GripperAngles__create();

/// Destroy msg/GripperAngles message.
/**
 * It calls
 * block_interfaces__msg__GripperAngles__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__GripperAngles__destroy(block_interfaces__msg__GripperAngles * msg);

/// Check for msg/GripperAngles message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__are_equal(const block_interfaces__msg__GripperAngles * lhs, const block_interfaces__msg__GripperAngles * rhs);

/// Copy a msg/GripperAngles message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__copy(
  const block_interfaces__msg__GripperAngles * input,
  block_interfaces__msg__GripperAngles * output);

/// Initialize array of msg/GripperAngles messages.
/**
 * It allocates the memory for the number of elements and calls
 * block_interfaces__msg__GripperAngles__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__Sequence__init(block_interfaces__msg__GripperAngles__Sequence * array, size_t size);

/// Finalize array of msg/GripperAngles messages.
/**
 * It calls
 * block_interfaces__msg__GripperAngles__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__GripperAngles__Sequence__fini(block_interfaces__msg__GripperAngles__Sequence * array);

/// Create array of msg/GripperAngles messages.
/**
 * It allocates the memory for the array and calls
 * block_interfaces__msg__GripperAngles__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__GripperAngles__Sequence *
block_interfaces__msg__GripperAngles__Sequence__create(size_t size);

/// Destroy array of msg/GripperAngles messages.
/**
 * It calls
 * block_interfaces__msg__GripperAngles__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__GripperAngles__Sequence__destroy(block_interfaces__msg__GripperAngles__Sequence * array);

/// Check for msg/GripperAngles message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__Sequence__are_equal(const block_interfaces__msg__GripperAngles__Sequence * lhs, const block_interfaces__msg__GripperAngles__Sequence * rhs);

/// Copy an array of msg/GripperAngles messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__GripperAngles__Sequence__copy(
  const block_interfaces__msg__GripperAngles__Sequence * input,
  block_interfaces__msg__GripperAngles__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__GRIPPER_ANGLES__FUNCTIONS_H_
