cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/grabbable_state_manager_test.cpp
  src/grabbable_state_queue_test.cpp
  src/grabbable_state_queues_manager_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
