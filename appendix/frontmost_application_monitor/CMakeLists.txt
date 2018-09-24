cmake_minimum_required (VERSION 3.9)

include (../../src/common.cmake)

project (a.out)

add_executable(
  a.out
  main.cpp
  ../../src/share/monitor/frontmost_application_monitor_objc.m
)

target_link_libraries(
  a.out
)
