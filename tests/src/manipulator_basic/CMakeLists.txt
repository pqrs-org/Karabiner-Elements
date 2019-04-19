cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/errors_test.cpp
  src/manipulator_basic_test.cpp
  src/simultaneous_options_test.cpp
  src/test.cpp
  src/to_after_key_up_test.cpp
  src/to_delayed_action_test.cpp
  src/to_if_alone_test.cpp
  src/to_if_held_down_test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
