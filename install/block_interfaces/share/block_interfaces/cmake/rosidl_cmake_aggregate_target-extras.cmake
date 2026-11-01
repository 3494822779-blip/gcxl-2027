# generated from rosidl_cmake/cmake/rosidl_cmake_aggregate_target-extras.cmake.in

# Create a convenience aggregate target block_interfaces::block_interfaces
# that links all generated interface targets, so downstream packages can use
# a single modern CMake target name instead of ${block_interfaces_TARGETS}.
if(block_interfaces_TARGETS AND NOT TARGET block_interfaces::block_interfaces)
  add_library(block_interfaces::block_interfaces INTERFACE IMPORTED)
  set_target_properties(block_interfaces::block_interfaces PROPERTIES
    INTERFACE_LINK_LIBRARIES "${block_interfaces_TARGETS}")
endif()
