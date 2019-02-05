cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  connected_devices_monitor_test.cpp
  test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
  "-framework CoreServices"
  "-framework SystemConfiguration"
)
