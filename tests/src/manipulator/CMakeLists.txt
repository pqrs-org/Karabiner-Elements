cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/manipulator_factory_test.cpp
  src/manipulator_manager_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
  "-framework CoreFoundation"
)

add_executable(
  overwrite_expected_results
  src/overwrite_expected_results.cpp
)

target_link_libraries(
  overwrite_expected_results
  test_runner
  "-framework CoreFoundation"
)
