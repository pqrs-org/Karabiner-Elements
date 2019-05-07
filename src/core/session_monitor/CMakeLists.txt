cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_session_monitor)

include_directories(
  ./include
)

add_executable(
  karabiner_session_monitor
  src/main.cpp
)

target_link_libraries(
  karabiner_session_monitor
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreServices"
)
