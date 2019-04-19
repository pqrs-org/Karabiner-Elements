cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

include_directories(../../../src/core/grabber/include)

add_executable(
  karabiner_test
  src/grabber_alerts_manager_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
