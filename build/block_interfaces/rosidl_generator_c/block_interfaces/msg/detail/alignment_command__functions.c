// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice
#include "block_interfaces/msg/detail/alignment_command__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"
// Member `reason`
#include "rosidl_runtime_c/string_functions.h"

bool
block_interfaces__msg__AlignmentCommand__init(block_interfaces__msg__AlignmentCommand * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    block_interfaces__msg__AlignmentCommand__fini(msg);
    return false;
  }
  // track_id
  // valid
  // predicted
  // error_x
  // error_y
  // yaw_rate
  // pitch_rate
  // aligned
  // grasp_request
  // valid_for_sec
  // reason
  if (!rosidl_runtime_c__String__init(&msg->reason)) {
    block_interfaces__msg__AlignmentCommand__fini(msg);
    return false;
  }
  return true;
}

void
block_interfaces__msg__AlignmentCommand__fini(block_interfaces__msg__AlignmentCommand * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // track_id
  // valid
  // predicted
  // error_x
  // error_y
  // yaw_rate
  // pitch_rate
  // aligned
  // grasp_request
  // valid_for_sec
  // reason
  rosidl_runtime_c__String__fini(&msg->reason);
}

bool
block_interfaces__msg__AlignmentCommand__are_equal(const block_interfaces__msg__AlignmentCommand * lhs, const block_interfaces__msg__AlignmentCommand * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__are_equal(
      &(lhs->header), &(rhs->header)))
  {
    return false;
  }
  // track_id
  if (lhs->track_id != rhs->track_id) {
    return false;
  }
  // valid
  if (lhs->valid != rhs->valid) {
    return false;
  }
  // predicted
  if (lhs->predicted != rhs->predicted) {
    return false;
  }
  // error_x
  if (lhs->error_x != rhs->error_x) {
    return false;
  }
  // error_y
  if (lhs->error_y != rhs->error_y) {
    return false;
  }
  // yaw_rate
  if (lhs->yaw_rate != rhs->yaw_rate) {
    return false;
  }
  // pitch_rate
  if (lhs->pitch_rate != rhs->pitch_rate) {
    return false;
  }
  // aligned
  if (lhs->aligned != rhs->aligned) {
    return false;
  }
  // grasp_request
  if (lhs->grasp_request != rhs->grasp_request) {
    return false;
  }
  // valid_for_sec
  if (lhs->valid_for_sec != rhs->valid_for_sec) {
    return false;
  }
  // reason
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->reason), &(rhs->reason)))
  {
    return false;
  }
  return true;
}

bool
block_interfaces__msg__AlignmentCommand__copy(
  const block_interfaces__msg__AlignmentCommand * input,
  block_interfaces__msg__AlignmentCommand * output)
{
  if (!input || !output) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__copy(
      &(input->header), &(output->header)))
  {
    return false;
  }
  // track_id
  output->track_id = input->track_id;
  // valid
  output->valid = input->valid;
  // predicted
  output->predicted = input->predicted;
  // error_x
  output->error_x = input->error_x;
  // error_y
  output->error_y = input->error_y;
  // yaw_rate
  output->yaw_rate = input->yaw_rate;
  // pitch_rate
  output->pitch_rate = input->pitch_rate;
  // aligned
  output->aligned = input->aligned;
  // grasp_request
  output->grasp_request = input->grasp_request;
  // valid_for_sec
  output->valid_for_sec = input->valid_for_sec;
  // reason
  if (!rosidl_runtime_c__String__copy(
      &(input->reason), &(output->reason)))
  {
    return false;
  }
  return true;
}

block_interfaces__msg__AlignmentCommand *
block_interfaces__msg__AlignmentCommand__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__AlignmentCommand * msg = (block_interfaces__msg__AlignmentCommand *)allocator.allocate(sizeof(block_interfaces__msg__AlignmentCommand), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(block_interfaces__msg__AlignmentCommand));
  bool success = block_interfaces__msg__AlignmentCommand__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
block_interfaces__msg__AlignmentCommand__destroy(block_interfaces__msg__AlignmentCommand * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    block_interfaces__msg__AlignmentCommand__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
block_interfaces__msg__AlignmentCommand__Sequence__init(block_interfaces__msg__AlignmentCommand__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__AlignmentCommand * data = NULL;

  if (size) {
    data = (block_interfaces__msg__AlignmentCommand *)allocator.zero_allocate(size, sizeof(block_interfaces__msg__AlignmentCommand), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = block_interfaces__msg__AlignmentCommand__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        block_interfaces__msg__AlignmentCommand__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
block_interfaces__msg__AlignmentCommand__Sequence__fini(block_interfaces__msg__AlignmentCommand__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      block_interfaces__msg__AlignmentCommand__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

block_interfaces__msg__AlignmentCommand__Sequence *
block_interfaces__msg__AlignmentCommand__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__AlignmentCommand__Sequence * array = (block_interfaces__msg__AlignmentCommand__Sequence *)allocator.allocate(sizeof(block_interfaces__msg__AlignmentCommand__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = block_interfaces__msg__AlignmentCommand__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
block_interfaces__msg__AlignmentCommand__Sequence__destroy(block_interfaces__msg__AlignmentCommand__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    block_interfaces__msg__AlignmentCommand__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
block_interfaces__msg__AlignmentCommand__Sequence__are_equal(const block_interfaces__msg__AlignmentCommand__Sequence * lhs, const block_interfaces__msg__AlignmentCommand__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!block_interfaces__msg__AlignmentCommand__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
block_interfaces__msg__AlignmentCommand__Sequence__copy(
  const block_interfaces__msg__AlignmentCommand__Sequence * input,
  block_interfaces__msg__AlignmentCommand__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(block_interfaces__msg__AlignmentCommand);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    block_interfaces__msg__AlignmentCommand * data =
      (block_interfaces__msg__AlignmentCommand *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!block_interfaces__msg__AlignmentCommand__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          block_interfaces__msg__AlignmentCommand__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!block_interfaces__msg__AlignmentCommand__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
