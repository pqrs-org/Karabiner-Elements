cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../common.cmake)

project (a.out LANGUAGES CXX OBJC)

add_executable(
  a.out
  ../../vendor/vendor/src/pqrs/osx/autorelease_pool/PQRSOSXAutoreleasePoolImpl.m
  main.cpp
)

set_source_files_properties(
  ../../vendor/vendor/src/pqrs/osx/autorelease_pool/PQRSOSXAutoreleasePoolImpl.m
  PROPERTIES
  COMPILE_OPTIONS "-fno-objc-arc"
)

target_link_libraries(
  a.out
  "-framework CoreFoundation"
  "-framework Foundation"
  "-framework IOKit"
)
