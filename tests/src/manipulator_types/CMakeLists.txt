cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/errors_test.cpp
  src/event_definition_test.cpp
  src/from_modifiers_definition_test.cpp
  src/modifier_definition_test.cpp
  src/test.cpp
)

target_link_libraries(
  karabiner_test
  test_runner
)
