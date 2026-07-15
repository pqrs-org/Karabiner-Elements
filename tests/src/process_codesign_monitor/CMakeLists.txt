cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../../tests.cmake)

project (karabiner_test LANGUAGES CXX OBJC)

add_executable(
  karabiner_test
  src/test.cpp
)

add_executable(
  karabiner_integration_test
  src/integration_test.cpp
)

add_executable(
  process_codesign_test_helper_a
  src/app/main.cpp
  src/app/pqrs_osx_application_impl.m
)

target_compile_definitions(
  process_codesign_test_helper_a
  PRIVATE PROCESS_CODESIGN_TEST_HELPER_VARIANT="A"
)

add_executable(
  process_codesign_test_helper_b
  src/app/main.cpp
  src/app/pqrs_osx_application_impl.m
)

target_compile_definitions(
  process_codesign_test_helper_b
  PRIVATE PROCESS_CODESIGN_TEST_HELPER_VARIANT="B"
)

foreach(target process_codesign_test_helper_a process_codesign_test_helper_b)
  target_compile_options(${target} PRIVATE -fno-sanitize=address)
  target_link_options(${target} PRIVATE -fno-sanitize=address)
  target_link_libraries(${target} "-framework AppKit")
  set_target_properties(${target} PROPERTIES OSX_ARCHITECTURES "arm64;x86_64")
endforeach()

target_link_libraries(
  karabiner_test
  "-framework CoreServices"
  "-framework Security"
)

target_link_libraries(
  karabiner_integration_test
  "-framework CoreServices"
  "-framework Security"
)

target_compile_definitions(
  karabiner_integration_test
  PRIVATE PROCESS_CODESIGN_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
