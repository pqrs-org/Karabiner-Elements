cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

include_directories(../../../src/core/grabber/include)

add_executable(
  karabiner_test
  post_event_to_virtual_devices_test.cpp
  test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
  "-framework CoreFoundation"
)

add_executable(
  overwrite_expected_results
  overwrite_expected_results.cpp
)

target_link_libraries(
  overwrite_expected_results
  test_runner
  "-framework CoreFoundation"
)
