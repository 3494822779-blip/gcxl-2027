// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from block_interfaces:msg/GripperAngles.idl
// generated code does not contain a copyright notice
#include "block_interfaces/msg/detail/gripper_angles__functions.h"

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
block_interfaces__msg__GripperAngles__init(block_interfaces__msg__GripperAngles * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    block_interfaces__msg__GripperAngles__fini(msg);
    return false;
  }
  // track_id
  // image_width
  // image_height
  // bounds_valid
  // left_edge_px
  // right_edge_px
  // angles_ready
  // left_angle_rad
  // right_angle_rad
  // valid_for_sec
  // reason
  if (!rosidl_runtime_c__String__init(&msg->reason)) {
    block_interfaces__msg__GripperAngles__fini(msg);
    return false;
  }
  return true;
}

void
block_interfaces__msg__GripperAngles__fini(block_interfaces__msg__GripperAngles * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // track_id
  // image_width
  // image_height
  // bounds_valid
  // left_edge_px
  // right_edge_px
  // angles_ready
  // left_angle_rad
  // right_angle_rad
  // valid_for_sec
  // reason
  rosidl_runtime_c__String__fini(&msg->reason);
}

bool
block_interfaces__msg__GripperAngles__are_equal(const block_interfaces__msg__GripperAngles * lhs, const block_interfaces__msg__GripperAngles * rhs)
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
  // image_width
  if (lhs->image_width != rhs->image_width) {
    return false;
  }
  // image_height
  if (lhs->image_height != rhs->image_height) {
    return false;
  }
  // bounds_valid
  if (lhs->bounds_valid != rhs->bounds_valid) {
    return false;
  }
  // left_edge_px
  if (lhs->left_edge_px != rhs->left_edge_px) {
    return false;
  }
  // right_edge_px
  if (lhs->right_edge_px != rhs->right_edge_px) {
    return false;
  }
  // angles_ready
  if (lhs->angles_ready != rhs->angles_ready) {
    return false;
  }
  // left_angle_rad
  if (lhs->left_angle_rad != rhs->left_angle_rad) {
    return false;
  }
  // right_angle_rad
  if (lhs->right_angle_rad != rhs->right_angle_rad) {
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
block_interfaces__msg__GripperAngles__copy(
  const block_interfaces__msg__GripperAngles * input,
  block_interfaces__msg__GripperAngles * output)
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
  // image_width
  output->image_width = input->image_width;
  // image_height
  output->image_height = input->image_height;
  // bounds_valid
  output->bounds_valid = input->bounds_valid;
  // left_edge_px
  output->left_edge_px = input->left_edge_px;
  // right_edge_px
  output->right_edge_px = input->right_edge_px;
  // angles_ready
  output->angles_ready = input->angles_ready;
  // left_angle_rad
  output->left_angle_rad = input->left_angle_rad;
  // right_angle_rad
  output->right_angle_rad = input->right_angle_rad;
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

block_interfaces__msg__GripperAngles *
block_interfaces__msg__GripperAngles__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__GripperAngles * msg = (block_interfaces__msg__GripperAngles *)allocator.allocate(sizeof(block_interfaces__msg__GripperAngles), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(block_interfaces__msg__GripperAngles));
  bool success = block_interfaces__msg__GripperAngles__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
block_interfaces__msg__GripperAngles__destroy(block_interfaces__msg__GripperAngles * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    block_interfaces__msg__GripperAngles__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
block_interfaces__msg__GripperAngles__Sequence__init(block_interfaces__msg__GripperAngles__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__GripperAngles * data = NULL;

  if (size) {
    data = (block_interfaces__msg__GripperAngles *)allocator.zero_allocate(size, sizeof(block_interfaces__msg__GripperAngles), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = block_interfaces__msg__GripperAngles__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        block_interfaces__msg__GripperAngles__fini(&data[i - 1]);
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
block_interfaces__msg__GripperAngles__Sequence__fini(block_interfaces__msg__GripperAngles__Sequence * array)
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
      block_interfaces__msg__GripperAngles__fini(&array->data[i]);
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

block_interfaces__msg__GripperAngles__Sequence *
block_interfaces__msg__GripperAngles__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  block_interfaces__msg__GripperAngles__Sequence * array = (block_interfaces__msg__GripperAngles__Sequence *)allocator.allocate(sizeof(block_interfaces__msg__GripperAngles__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = block_interfaces__msg__GripperAngles__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
block_interfaces__msg__GripperAngles__Sequence__destroy(block_interfaces__msg__GripperAngles__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    block_interfaces__msg__GripperAngles__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
block_interfaces__msg__GripperAngles__Sequence__are_equal(const block_interfaces__msg__GripperAngles__Sequence * lhs, const block_interfaces__msg__GripperAngles__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!block_interfaces__msg__GripperAngles__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
block_interfaces__msg__GripperAngles__Sequence__copy(
  const block_interfaces__msg__GripperAngles__Sequence * input,
  block_interfaces__msg__GripperAngles__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(block_interfaces__msg__GripperAngles);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    block_interfaces__msg__GripperAngles * data =
      (block_interfaces__msg__GripperAngles *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!block_interfaces__msg__GripperAngles__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          block_interfaces__msg__GripperAngles__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!block_interfaces__msg__GripperAngles__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
