cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/event_queue_test.cpp
  src/event_queue_event_time_stamp_test.cpp
  src/event_queue_utility_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
