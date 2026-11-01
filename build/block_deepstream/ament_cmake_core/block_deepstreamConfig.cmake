# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_block_deepstream_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED block_deepstream_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(block_deepstream_FOUND FALSE)
  elseif(NOT block_deepstream_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(block_deepstream_FOUND FALSE)
  endif()
  return()
endif()
set(_block_deepstream_CONFIG_INCLUDED TRUE)

# output package information
if(NOT block_deepstream_FIND_QUIETLY)
  message(STATUS "Found block_deepstream: 0.1.0 (${block_deepstream_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'block_deepstream' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT ${block_deepstream_DEPRECATED_QUIET})
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(block_deepstream_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${block_deepstream_DIR}/${_extra}")
endforeach()
