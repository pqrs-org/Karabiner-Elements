cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_grabber)

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
  ./include
)

add_executable(
  karabiner_grabber
  src/main.cpp
)

target_link_libraries(
  karabiner_grabber
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreServices"
  "-framework IOKit"
  "-framework SystemConfiguration"
)
