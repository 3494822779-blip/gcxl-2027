// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice

#ifndef BLOCK_INTERFACES__MSG__DETAIL__FRAME__FUNCTIONS_H_
#define BLOCK_INTERFACES__MSG__DETAIL__FRAME__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "block_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "block_interfaces/msg/detail/frame__struct.h"

/// Initialize msg/Frame message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * block_interfaces__msg__Frame
 * )) before or use
 * block_interfaces__msg__Frame__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__Frame__init(block_interfaces__msg__Frame * msg);

/// Finalize msg/Frame message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__Frame__fini(block_interfaces__msg__Frame * msg);

/// Create msg/Frame message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * block_interfaces__msg__Frame__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__Frame *
block_interfaces__msg__Frame__create();

/// Destroy msg/Frame message.
/**
 * It calls
 * block_interfaces__msg__Frame__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__Frame__destroy(block_interfaces__msg__Frame * msg);

/// Check for msg/Frame message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__Frame__are_equal(const block_interfaces__msg__Frame * lhs, const block_interfaces__msg__Frame * rhs);

/// Copy a msg/Frame message.
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
block_interfaces__msg__Frame__copy(
  const block_interfaces__msg__Frame * input,
  block_interfaces__msg__Frame * output);

/// Initialize array of msg/Frame messages.
/**
 * It allocates the memory for the number of elements and calls
 * block_interfaces__msg__Frame__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__Frame__Sequence__init(block_interfaces__msg__Frame__Sequence * array, size_t size);

/// Finalize array of msg/Frame messages.
/**
 * It calls
 * block_interfaces__msg__Frame__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__Frame__Sequence__fini(block_interfaces__msg__Frame__Sequence * array);

/// Create array of msg/Frame messages.
/**
 * It allocates the memory for the array and calls
 * block_interfaces__msg__Frame__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
block_interfaces__msg__Frame__Sequence *
block_interfaces__msg__Frame__Sequence__create(size_t size);

/// Destroy array of msg/Frame messages.
/**
 * It calls
 * block_interfaces__msg__Frame__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
void
block_interfaces__msg__Frame__Sequence__destroy(block_interfaces__msg__Frame__Sequence * array);

/// Check for msg/Frame message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_block_interfaces
bool
block_interfaces__msg__Frame__Sequence__are_equal(const block_interfaces__msg__Frame__Sequence * lhs, const block_interfaces__msg__Frame__Sequence * rhs);

/// Copy an array of msg/Frame messages.
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
block_interfaces__msg__Frame__Sequence__copy(
  const block_interfaces__msg__Frame__Sequence * input,
  block_interfaces__msg__Frame__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_INTERFACES__MSG__DETAIL__FRAME__FUNCTIONS_H_
