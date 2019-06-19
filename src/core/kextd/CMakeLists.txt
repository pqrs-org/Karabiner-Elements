cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_kextd)

include_directories(
  ./include
)

add_executable(
  karabiner_kextd
  src/main.cpp
)

target_link_libraries(
  karabiner_kextd
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreServices"
  "-framework IOKit"
  "-framework SystemConfiguration"
  c++
)
