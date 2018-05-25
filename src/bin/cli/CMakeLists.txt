cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_cli)

add_executable(
  karabiner_cli
  src/main.cpp
)

target_link_libraries(
  karabiner_cli
  "-framework CoreFoundation"
  "-framework CoreServices"
  "-framework SystemConfiguration"
)
