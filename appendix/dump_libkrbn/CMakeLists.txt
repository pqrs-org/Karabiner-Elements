cmake_minimum_required (VERSION 3.9)

include (../common.cmake)

project (a.out)

link_directories(
  ../../src/lib/libkrbn/build/Release
)

add_executable(
  a.out
  main.cpp
)

target_link_libraries(
  a.out
  krbn
  "-framework CoreFoundation"
  "-framework CoreServices"
  "-framework IOKit"
  "-framework SystemConfiguration"
)
