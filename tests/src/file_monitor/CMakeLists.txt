cmake_minimum_required (VERSION 3.9)

include (../../../src/common.cmake)

project (test)

add_executable(
  test
  test.cpp
)

target_link_libraries(
  test
  "-framework CoreServices"
)
