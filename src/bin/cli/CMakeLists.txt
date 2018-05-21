cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_cli)

add_compile_options(
  -Wall
  -Werror
  -O2
  -std=c++14
)

include_directories(
  /opt/local/include
  /usr/local/include
  ../../share
  ../../vendor
)

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
