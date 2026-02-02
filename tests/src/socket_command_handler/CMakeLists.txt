cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include(../../tests.cmake)

project(karabiner_test)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../src/core/console_user_server/include)

add_executable(
  karabiner_test
  src/test.cpp
)

