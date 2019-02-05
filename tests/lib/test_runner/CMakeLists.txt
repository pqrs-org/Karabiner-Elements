cmake_minimum_required (VERSION 3.9)

include (../../tests.cmake)

project (test_runner)

add_library(
  test_runner
  src/test_runner.cpp
)
