// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__FUNCTIONS_H_
#define BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "block_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "block_interfaces/msg/detail/alignment_command__struct.h"

/// Initialize msg/AlignmentCommand message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * block_interfaces__msg__AlignmentCommand
 * )) before or use
 * block_interfaces__msg__AlignmentCommand__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__AlignmentCommand__init(block_interfaces__msg__AlignmentCommand * msg);

/// Finalize msg/AlignmentCommand message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__AlignmentCommand__fini(block_interfaces__msg__AlignmentCommand * msg);

/// Create msg/AlignmentCommand message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * block_interfaces__msg__AlignmentCommand__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__AlignmentCommand *
block_interfaces__msg__AlignmentCommand__create();

/// Destroy msg/AlignmentCommand message.
/**
 * It calls
 * block_interfaces__msg__AlignmentCommand__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__AlignmentCommand__destroy(block_interfaces__msg__AlignmentCommand * msg);

/// Check for msg/AlignmentCommand message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__AlignmentCommand__are_equal(const block_interfaces__msg__AlignmentCommand * lhs, const block_interfaces__msg__AlignmentCommand * rhs);

/// Copy a msg/AlignmentCommand message.
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
block_interfaces__msg__AlignmentCommand__copy(
  const block_interfaces__msg__AlignmentCommand * input,
  block_interfaces__msg__AlignmentCommand * output);

/// Initialize array of msg/AlignmentCommand messages.
/**
 * It allocates the memory for the number of elements and calls
 * block_interfaces__msg__AlignmentCommand__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__AlignmentCommand__Sequence__init(block_interfaces__msg__AlignmentCommand__Sequence * array, size_t size);

/// Finalize array of msg/AlignmentCommand messages.
/**
 * It calls
 * block_interfaces__msg__AlignmentCommand__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__AlignmentCommand__Sequence__fini(block_interfaces__msg__AlignmentCommand__Sequence * array);

/// Create array of msg/AlignmentCommand messages.
/**
 * It allocates the memory for the array and calls
 * block_interfaces__msg__AlignmentCommand__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__AlignmentCommand__Sequence *
block_interfaces__msg__AlignmentCommand__Sequence__create(size_t size);

/// Destroy array of msg/AlignmentCommand messages.
/**
 * It calls
 * block_interfaces__msg__AlignmentCommand__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__AlignmentCommand__Sequence__destroy(block_interfaces__msg__AlignmentCommand__Sequence * array);

/// Check for msg/AlignmentCommand message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__AlignmentCommand__Sequence__are_equal(const block_interfaces__msg__AlignmentCommand__Sequence * lhs, const block_interfaces__msg__AlignmentCommand__Sequence * rhs);

/// Copy an array of msg/AlignmentCommand messages.
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
block_interfaces__msg__AlignmentCommand__Sequence__copy(
  const block_interfaces__msg__AlignmentCommand__Sequence * input,
  block_interfaces__msg__AlignmentCommand__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__ALIGNMENT_COMMAND__FUNCTIONS_H_
