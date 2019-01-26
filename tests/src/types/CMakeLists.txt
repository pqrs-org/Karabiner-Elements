cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/device_identifiers_test.cpp
  src/input_source_selector_test.cpp
  src/mouse_key_test.cpp
  src/operation_type_test.cpp
  src/pointing_motion_test.cpp
  src/test.cpp
)
