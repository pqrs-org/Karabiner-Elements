cmake_minimum_required (VERSION 3.9)

include (../../../src/common.cmake)

project (karabiner_test)

include_directories(../../../src/core/grabber/include)

add_executable(
  karabiner_test
  test.cpp
)
