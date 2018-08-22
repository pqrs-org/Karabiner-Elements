cmake_minimum_required (VERSION 3.9)

include (../../../src/common.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  test.cpp
)

target_link_libraries(
  karabiner_test
  "-framework CoreServices"
  "-framework SystemConfiguration"
)
