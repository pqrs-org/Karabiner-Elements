cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/device_identifiers_test.cpp
  src/errors_test.cpp
  src/grabbable_state_test.cpp
  src/key_down_up_valued_event_test.cpp
  src/mouse_key_test.cpp
  src/operation_type_test.cpp
  src/pointing_motion_test.cpp
  src/test.cpp
  src/types_test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
