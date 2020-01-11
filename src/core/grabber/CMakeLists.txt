cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_grabber)

include_directories(
  ./include
)

add_executable(
  karabiner_grabber
  src/main.cpp
)

set_target_properties(
  karabiner_grabber
  PROPERTIES

  XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME
  YES
)

target_link_libraries(
  karabiner_grabber
  "-framework CoreFoundation"
  "-framework CoreGraphics"
  "-framework CoreServices"
  "-framework IOKit"
  "-framework SystemConfiguration"
)
