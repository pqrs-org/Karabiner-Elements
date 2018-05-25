cmake_minimum_required (VERSION 3.9)

include (../../../src/common.cmake)

project (test)

include_directories(../../../src/core/grabber/include)

add_executable(
  test
  test.cpp
)

target_link_libraries(
  test
  "-framework CoreFoundation"
)

add_executable(
  overwrite_expected_results
  overwrite_expected_results.cpp
)

target_link_libraries(
  overwrite_expected_results
  "-framework CoreFoundation"
)
