// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from block_interfaces:msg/Target.idl
// generated code does not contain a copyright notice
#include "block_interfaces/msg/detail/target__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"
// Member `last_observed`
#include "builtin_interfaces/msg/detail/time__functions.h"

bool
block_interfaces__msg__Target__init(block_interfaces__msg__Target * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    block_interfaces__msg__Target__fini(msg);
    return false;
  }
  // image_width
  // image_height
  // locked
  // observed
  // valid
  // track_id
  // class_id
  // raw_x
  // raw_y
  // x
  // y
  // last_observed
  if (!builtin_interfaces__msg__Time__init(&msg->last_observed)) {
    block_interfaces__msg__Target__fini(msg);
    return false;
  }
  return true;
}

void
block_interfaces__msg__Target__fini(block_interfaces__msg__Target * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // image_width
  // image_height
  // locked
  // observed
  // valid
  // track_id
  // class_id
  // raw_x
  // raw_y
  // x
  // y
  // last_observed
  builtin_interfaces__msg__Time__fini(&msg->last_observed);
}

bool
block_interfaces__msg__Target__are_equal(const block_interfaces__msg__Target * lhs, const block_interfaces__msg__Target * rhs)
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
  // image_width
  if (lhs->image_width != rhs->image_width) {
    return false;
  }
  // image_height
  if (lhs->image_height != rhs->image_height) {
    return false;
  }
  // locked
  if (lhs->locked != rhs->locked) {
    return false;
  }
  // observed
  if (lhs->observed != rhs->observed) {
    return false;
  }
  // valid
  if (lhs->valid != rhs->valid) {
    return false;
  }
  // track_id
  if (lhs->track_id != rhs->track_id) {
    return false;
  }
  // class_id
  if (lhs->class_id != rhs->class_id) {
    return false;
  }
  // raw_x
  if (lhs->raw_x != rhs->raw_x) {
    return false;
  }
  // raw_y
  if (lhs->raw_y != rhs->raw_y) {
    return false;
  }
  // x
  if (lhs->x != rhs->x) {
    return false;
  }
  // y
  if (lhs->y != rhs->y) {
    return false;
  }
  // last_observed
  if (!builtin_interfaces__msg__Time__are_equal(
      &(lhs->last_observed), &(rhs->last_observed)))
  {
    return false;
  }
  return true;
}

bool
block_interfaces__msg__Target__copy(
  const block_interfaces__msg__Target * input,
  block_interfaces__msg__Target * output)
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
  // image_width
  output->image_width = input->image_width;
  // image_height
  output->image_height = input->image_height;
  // locked
  output->locked = input->locked;
  // observed
  output->observed = input->observed;
  // valid
  output->valid = input->valid;
  // track_id
  output->track_id = input->track_id;
  // class_id
  output->class_id = input->class_id;
  // raw_x
  output->raw_x = input->raw_x;
  // raw_y
  output->raw_y = input->raw_y;
  // x
  output->x = input->x;
  // y
  output->y = input->y;
  // last_observed
  if (!builtin_interfaces__msg__Time__copy(
      &(input->last_observed), &(output->last_observed)))
  {
    return false;
  }
  return true;
}

block_interfaces__msg__Target *
block_interfaces__msg__Target__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Target * msg = (block_interfaces__msg__Target *)allocator.allocate(sizeof(block_interfaces__msg__Target), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(block_interfaces__msg__Target));
  bool success = block_interfaces__msg__Target__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
block_interfaces__msg__Target__destroy(block_interfaces__msg__Target * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    block_interfaces__msg__Target__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
block_interfaces__msg__Target__Sequence__init(block_interfaces__msg__Target__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Target * data = NULL;

  if (size) {
    data = (block_interfaces__msg__Target *)allocator.zero_allocate(size, sizeof(block_interfaces__msg__Target), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = block_interfaces__msg__Target__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        block_interfaces__msg__Target__fini(&data[i - 1]);
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
block_interfaces__msg__Target__Sequence__fini(block_interfaces__msg__Target__Sequence * array)
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
      block_interfaces__msg__Target__fini(&array->data[i]);
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

block_interfaces__msg__Target__Sequence *
block_interfaces__msg__Target__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Target__Sequence * array = (block_interfaces__msg__Target__Sequence *)allocator.allocate(sizeof(block_interfaces__msg__Target__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = block_interfaces__msg__Target__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
block_interfaces__msg__Target__Sequence__destroy(block_interfaces__msg__Target__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    block_interfaces__msg__Target__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
block_interfaces__msg__Target__Sequence__are_equal(const block_interfaces__msg__Target__Sequence * lhs, const block_interfaces__msg__Target__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!block_interfaces__msg__Target__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
block_interfaces__msg__Target__Sequence__copy(
  const block_interfaces__msg__Target__Sequence * input,
  block_interfaces__msg__Target__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(block_interfaces__msg__Target);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    block_interfaces__msg__Target * data =
      (block_interfaces__msg__Target *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!block_interfaces__msg__Target__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          block_interfaces__msg__Target__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!block_interfaces__msg__Target__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
