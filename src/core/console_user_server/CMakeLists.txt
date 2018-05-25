cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_console_user_server)

include_directories(
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
