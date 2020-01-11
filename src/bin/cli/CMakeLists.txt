cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_cli)

add_executable(
  karabiner_cli
  src/main.cpp
)

set_target_properties(
  karabiner_cli
  PROPERTIES

  XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME
  YES
)

target_link_libraries(
  karabiner_cli
  "-framework CoreFoundation"
  "-framework CoreServices"
  "-framework SystemConfiguration"
)
