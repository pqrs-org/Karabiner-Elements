cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/complex_modifications_assets_file_test.cpp
  src/complex_modifications_assets_manager_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
