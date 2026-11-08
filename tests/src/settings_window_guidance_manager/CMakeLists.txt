cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../../tests.cmake)

project (karabiner_test)

enable_language(OBJC)

add_executable(
  karabiner_test
  src/test.cpp
)

enable_pqrs_osx_autorelease_pool(karabiner_test)
