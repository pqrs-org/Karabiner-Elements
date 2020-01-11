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

set_target_properties(
  karabiner_kextd
  PROPERTIES

  XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME
  YES
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
