// generated from rosidl_generator_py/resource/_idl_support.c.em
// with input from block_interfaces:msg/AlignmentCommand.idl
// generated code does not contain a copyright notice
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <stdbool.h>
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "numpy/ndarrayobject.h"
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif
#include "rosidl_runtime_c/visibility_control.h"
#include "block_interfaces/msg/detail/alignment_command__struct.h"
#include "block_interfaces/msg/detail/alignment_command__functions.h"

#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/string_functions.h"

ROSIDL_GENERATOR_C_IMPORT
bool std_msgs__msg__header__convert_from_py(PyObject * _pymsg, void * _ros_message);
ROSIDL_GENERATOR_C_IMPORT
PyObject * std_msgs__msg__header__convert_to_py(void * raw_ros_message);

ROSIDL_GENERATOR_C_EXPORT
bool block_interfaces__msg__alignment_command__convert_from_py(PyObject * _pymsg, void * _ros_message)
{
  // check that the passed message is of the expected Python class
  {
    char full_classname_dest[57];
    {
      char * class_name = NULL;
      char * module_name = NULL;
      {
        PyObject * class_attr = PyObject_GetAttrString(_pymsg, "__class__");
        if (class_attr) {
          PyObject * name_attr = PyObject_GetAttrString(class_attr, "__name__");
          if (name_attr) {
            class_name = (char *)PyUnicode_1BYTE_DATA(name_attr);
            Py_DECREF(name_attr);
          }
          PyObject * module_attr = PyObject_GetAttrString(class_attr, "__module__");
          if (module_attr) {
            module_name = (char *)PyUnicode_1BYTE_DATA(module_attr);
            Py_DECREF(module_attr);
          }
          Py_DECREF(class_attr);
        }
      }
      if (!class_name || !module_name) {
        return false;
      }
      snprintf(full_classname_dest, sizeof(full_classname_dest), "%s.%s", module_name, class_name);
    }
    assert(strncmp("block_interfaces.msg._alignment_command.AlignmentCommand", full_classname_dest, 56) == 0);
  }
  block_interfaces__msg__AlignmentCommand * ros_message = _ros_message;
  {  // header
    PyObject * field = PyObject_GetAttrString(_pymsg, "header");
    if (!field) {
      return false;
    }
    if (!std_msgs__msg__header__convert_from_py(field, &ros_message->header)) {
      Py_DECREF(field);
      return false;
    }
    Py_DECREF(field);
  }
  {  // track_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "track_id");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->track_id = PyLong_AsUnsignedLongLong(field);
    Py_DECREF(field);
  }
  {  // valid
    PyObject * field = PyObject_GetAttrString(_pymsg, "valid");
    if (!field) {
      return false;
    }
    assert(PyBool_Check(field));
    ros_message->valid = (Py_True == field);
    Py_DECREF(field);
  }
  {  // predicted
    PyObject * field = PyObject_GetAttrString(_pymsg, "predicted");
    if (!field) {
      return false;
    }
    assert(PyBool_Check(field));
    ros_message->predicted = (Py_True == field);
    Py_DECREF(field);
  }
  {  // error_x
    PyObject * field = PyObject_GetAttrString(_pymsg, "error_x");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->error_x = PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // error_y
    PyObject * field = PyObject_GetAttrString(_pymsg, "error_y");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->error_y = PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // yaw_rate
    PyObject * field = PyObject_GetAttrString(_pymsg, "yaw_rate");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->yaw_rate = PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // pitch_rate
    PyObject * field = PyObject_GetAttrString(_pymsg, "pitch_rate");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->pitch_rate = PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // aligned
    PyObject * field = PyObject_GetAttrString(_pymsg, "aligned");
    if (!field) {
      return false;
    }
    assert(PyBool_Check(field));
    ros_message->aligned = (Py_True == field);
    Py_DECREF(field);
  }
  {  // grasp_request
    PyObject * field = PyObject_GetAttrString(_pymsg, "grasp_request");
    if (!field) {
      return false;
    }
    assert(PyBool_Check(field));
    ros_message->grasp_request = (Py_True == field);
    Py_DECREF(field);
  }
  {  // valid_for_sec
    PyObject * field = PyObject_GetAttrString(_pymsg, "valid_for_sec");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->valid_for_sec = PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // reason
    PyObject * field = PyObject_GetAttrString(_pymsg, "reason");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->reason, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }

  return true;
}

ROSIDL_GENERATOR_C_EXPORT
PyObject * block_interfaces__msg__alignment_command__convert_to_py(void * raw_ros_message)
{
  /* NOTE(esteve): Call constructor of AlignmentCommand */
  PyObject * _pymessage = NULL;
  {
    PyObject * pymessage_module = PyImport_ImportModule("block_interfaces.msg._alignment_command");
    assert(pymessage_module);
    PyObject * pymessage_class = PyObject_GetAttrString(pymessage_module, "AlignmentCommand");
    assert(pymessage_class);
    Py_DECREF(pymessage_module);
    _pymessage = PyObject_CallObject(pymessage_class, NULL);
    Py_DECREF(pymessage_class);
    if (!_pymessage) {
      return NULL;
    }
  }
  block_interfaces__msg__AlignmentCommand * ros_message = (block_interfaces__msg__AlignmentCommand *)raw_ros_message;
  {  // header
    PyObject * field = NULL;
    field = std_msgs__msg__header__convert_to_py(&ros_message->header);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "header", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // track_id
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLongLong(ros_message->track_id);
    {
      int rc = PyObject_SetAttrString(_pymessage, "track_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // valid
    PyObject * field = NULL;
    field = PyBool_FromLong(ros_message->valid ? 1 : 0);
    {
      int rc = PyObject_SetAttrString(_pymessage, "valid", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // predicted
    PyObject * field = NULL;
    field = PyBool_FromLong(ros_message->predicted ? 1 : 0);
    {
      int rc = PyObject_SetAttrString(_pymessage, "predicted", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // error_x
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->error_x);
    {
      int rc = PyObject_SetAttrString(_pymessage, "error_x", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // error_y
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->error_y);
    {
      int rc = PyObject_SetAttrString(_pymessage, "error_y", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // yaw_rate
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->yaw_rate);
    {
      int rc = PyObject_SetAttrString(_pymessage, "yaw_rate", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // pitch_rate
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->pitch_rate);
    {
      int rc = PyObject_SetAttrString(_pymessage, "pitch_rate", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // aligned
    PyObject * field = NULL;
    field = PyBool_FromLong(ros_message->aligned ? 1 : 0);
    {
      int rc = PyObject_SetAttrString(_pymessage, "aligned", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // grasp_request
    PyObject * field = NULL;
    field = PyBool_FromLong(ros_message->grasp_request ? 1 : 0);
    {
      int rc = PyObject_SetAttrString(_pymessage, "grasp_request", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // valid_for_sec
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->valid_for_sec);
    {
      int rc = PyObject_SetAttrString(_pymessage, "valid_for_sec", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // reason
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->reason.data,
      strlen(ros_message->reason.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "reason", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }

  // ownership of _pymessage is transferred to the caller
  return _pymessage;
}
