cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  grabbable_state_manager_test.cpp
  grabbable_state_queue_test.cpp
  grabbable_state_queues_manager_test.cpp
  test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
