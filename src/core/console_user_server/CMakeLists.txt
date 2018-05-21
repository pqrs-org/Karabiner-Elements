cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_console_user_server)

add_compile_options(
  -Wall
  -Werror
  -O2
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>
  $<$<COMPILE_LANGUAGE:C>:-fobjc-arc>
)

include_directories(
  /opt/local/include
  /usr/local/include
  ../../share
  ../../vendor
  ./include
)

add_executable(
  karabiner_console_user_server
  src/main.cpp
  ../../share/frontmost_application_observer_objc.m
)

target_link_libraries(
  karabiner_console_user_server
  "-framework Carbon"
  "-framework Cocoa"
  "-framework SystemConfiguration"
)
