// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from block_interfaces:msg/Frame.idl
// generated code does not contain a copyright notice
#include "block_interfaces/msg/detail/frame__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `image`
#include "sensor_msgs/msg/detail/image__functions.h"
// Member `boxes`
#include "block_interfaces/msg/detail/box__functions.h"

bool
block_interfaces__msg__Frame__init(block_interfaces__msg__Frame * msg)
{
  if (!msg) {
    return false;
  }
  // image
  if (!sensor_msgs__msg__Image__init(&msg->image)) {
    block_interfaces__msg__Frame__fini(msg);
    return false;
  }
  // boxes
  if (!block_interfaces__msg__Box__Sequence__init(&msg->boxes, 0)) {
    block_interfaces__msg__Frame__fini(msg);
    return false;
  }
  return true;
}

void
block_interfaces__msg__Frame__fini(block_interfaces__msg__Frame * msg)
{
  if (!msg) {
    return;
  }
  // image
  sensor_msgs__msg__Image__fini(&msg->image);
  // boxes
  block_interfaces__msg__Box__Sequence__fini(&msg->boxes);
}

bool
block_interfaces__msg__Frame__are_equal(const block_interfaces__msg__Frame * lhs, const block_interfaces__msg__Frame * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // image
  if (!sensor_msgs__msg__Image__are_equal(
      &(lhs->image), &(rhs->image)))
  {
    return false;
  }
  // boxes
  if (!block_interfaces__msg__Box__Sequence__are_equal(
      &(lhs->boxes), &(rhs->boxes)))
  {
    return false;
  }
  return true;
}

bool
block_interfaces__msg__Frame__copy(
  const block_interfaces__msg__Frame * input,
  block_interfaces__msg__Frame * output)
{
  if (!input || !output) {
    return false;
  }
  // image
  if (!sensor_msgs__msg__Image__copy(
      &(input->image), &(output->image)))
  {
    return false;
  }
  // boxes
  if (!block_interfaces__msg__Box__Sequence__copy(
      &(input->boxes), &(output->boxes)))
  {
    return false;
  }
  return true;
}

block_interfaces__msg__Frame *
block_interfaces__msg__Frame__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Frame * msg = (block_interfaces__msg__Frame *)allocator.allocate(sizeof(block_interfaces__msg__Frame), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(block_interfaces__msg__Frame));
  bool success = block_interfaces__msg__Frame__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
block_interfaces__msg__Frame__destroy(block_interfaces__msg__Frame * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    block_interfaces__msg__Frame__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
block_interfaces__msg__Frame__Sequence__init(block_interfaces__msg__Frame__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Frame * data = NULL;

  if (size) {
    data = (block_interfaces__msg__Frame *)allocator.zero_allocate(size, sizeof(block_interfaces__msg__Frame), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = block_interfaces__msg__Frame__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        block_interfaces__msg__Frame__fini(&data[i - 1]);
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
block_interfaces__msg__Frame__Sequence__fini(block_interfaces__msg__Frame__Sequence * array)
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
      block_interfaces__msg__Frame__fini(&array->data[i]);
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

block_interfaces__msg__Frame__Sequence *
block_interfaces__msg__Frame__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Frame__Sequence * array = (block_interfaces__msg__Frame__Sequence *)allocator.allocate(sizeof(block_interfaces__msg__Frame__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = block_interfaces__msg__Frame__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
block_interfaces__msg__Frame__Sequence__destroy(block_interfaces__msg__Frame__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    block_interfaces__msg__Frame__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
block_interfaces__msg__Frame__Sequence__are_equal(const block_interfaces__msg__Frame__Sequence * lhs, const block_interfaces__msg__Frame__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!block_interfaces__msg__Frame__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
block_interfaces__msg__Frame__Sequence__copy(
  const block_interfaces__msg__Frame__Sequence * input,
  block_interfaces__msg__Frame__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(block_interfaces__msg__Frame);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    block_interfaces__msg__Frame * data =
      (block_interfaces__msg__Frame *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!block_interfaces__msg__Frame__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          block_interfaces__msg__Frame__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!block_interfaces__msg__Frame__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
