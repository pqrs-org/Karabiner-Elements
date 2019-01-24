cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (krbn)

include_directories(
  ./include
  ./impl/include
)

add_library(
  krbn
  src/libkrbn_configuration.cpp
  src/libkrbn_connected_devices.cpp
  src/libkrbn_log_lines.cpp
  src/libkrbn.cpp
  ../../vendor/cget/src/pqrs/osx/frontmost_application_monitor/impl/objc.m
)
