cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../common.cmake)

project (a.out)

add_executable(
  a.out
  main.cpp
)

target_link_libraries(
  a.out
  "-framework CoreServices"
)
