cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

include_directories(../../../src/core/grabber/include)

add_executable(
  karabiner_test
  src/device_test.cpp
  src/global_configuration_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  "-framework CoreFoundation"
  "-framework SystemConfiguration"
)
