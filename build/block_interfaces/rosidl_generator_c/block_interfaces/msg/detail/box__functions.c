// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from block_interfaces:msg/Box.idl
// generated code does not contain a copyright notice
#include "block_interfaces/msg/detail/box__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
block_interfaces__msg__Box__init(block_interfaces__msg__Box * msg)
{
  if (!msg) {
    return false;
  }
  // track_id
  // class_id
  // confidence
  // x
  // y
  // width
  // height
  // observed
  return true;
}

void
block_interfaces__msg__Box__fini(block_interfaces__msg__Box * msg)
{
  if (!msg) {
    return;
  }
  // track_id
  // class_id
  // confidence
  // x
  // y
  // width
  // height
  // observed
}

bool
block_interfaces__msg__Box__are_equal(const block_interfaces__msg__Box * lhs, const block_interfaces__msg__Box * rhs)
{
  if (!lhs || !rhs) {
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
  // confidence
  if (lhs->confidence != rhs->confidence) {
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
  // width
  if (lhs->width != rhs->width) {
    return false;
  }
  // height
  if (lhs->height != rhs->height) {
    return false;
  }
  // observed
  if (lhs->observed != rhs->observed) {
    return false;
  }
  return true;
}

bool
block_interfaces__msg__Box__copy(
  const block_interfaces__msg__Box * input,
  block_interfaces__msg__Box * output)
{
  if (!input || !output) {
    return false;
  }
  // track_id
  output->track_id = input->track_id;
  // class_id
  output->class_id = input->class_id;
  // confidence
  output->confidence = input->confidence;
  // x
  output->x = input->x;
  // y
  output->y = input->y;
  // width
  output->width = input->width;
  // height
  output->height = input->height;
  // observed
  output->observed = input->observed;
  return true;
}

block_interfaces__msg__Box *
block_interfaces__msg__Box__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Box * msg = (block_interfaces__msg__Box *)allocator.allocate(sizeof(block_interfaces__msg__Box), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(block_interfaces__msg__Box));
  bool success = block_interfaces__msg__Box__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
block_interfaces__msg__Box__destroy(block_interfaces__msg__Box * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    block_interfaces__msg__Box__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
block_interfaces__msg__Box__Sequence__init(block_interfaces__msg__Box__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Box * data = NULL;

  if (size) {
    data = (block_interfaces__msg__Box *)allocator.zero_allocate(size, sizeof(block_interfaces__msg__Box), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = block_interfaces__msg__Box__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        block_interfaces__msg__Box__fini(&data[i - 1]);
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
block_interfaces__msg__Box__Sequence__fini(block_interfaces__msg__Box__Sequence * array)
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
      block_interfaces__msg__Box__fini(&array->data[i]);
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

block_interfaces__msg__Box__Sequence *
block_interfaces__msg__Box__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__Box__Sequence * array = (block_interfaces__msg__Box__Sequence *)allocator.allocate(sizeof(block_interfaces__msg__Box__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = block_interfaces__msg__Box__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
block_interfaces__msg__Box__Sequence__destroy(block_interfaces__msg__Box__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    block_interfaces__msg__Box__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
block_interfaces__msg__Box__Sequence__are_equal(const block_interfaces__msg__Box__Sequence * lhs, const block_interfaces__msg__Box__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!block_interfaces__msg__Box__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
block_interfaces__msg__Box__Sequence__copy(
  const block_interfaces__msg__Box__Sequence * input,
  block_interfaces__msg__Box__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(block_interfaces__msg__Box);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    block_interfaces__msg__Box * data =
      (block_interfaces__msg__Box *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!block_interfaces__msg__Box__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          block_interfaces__msg__Box__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!block_interfaces__msg__Box__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
