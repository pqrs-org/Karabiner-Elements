cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/compare_test.cpp
  src/device_identifiers_test.cpp
  src/device_properties_test.cpp
  src/test.cpp
  src/to_json_test.cpp
)

target_link_libraries(
  karabiner_test
  "-framework CoreFoundation"
  "-framework IOKit"
)
