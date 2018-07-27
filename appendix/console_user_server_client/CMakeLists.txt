cmake_minimum_required (VERSION 3.9)

include (../../src/common.cmake)

project (a.out)

include_directories(../../src/core/grabber/include)

add_executable(
  a.out
  main.cpp
)

target_link_libraries(
  a.out
  "-framework CoreFoundation"
  "-framework SystemConfiguration"
)
