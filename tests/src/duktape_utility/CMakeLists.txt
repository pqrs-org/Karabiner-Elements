cmake_minimum_required(VERSION 3.9)

include(../../tests.cmake)

project(karabiner_test)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../src/vendor/duktape-2.7.0/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../src/vendor/duktape-2.7.0/extras/console)
include_directories(${CMAKE_CURRENT_LIST_DIR}/../../../src/vendor/duktape-2.7.0/extras/module-node)

# Suppress duktape warnings
add_compile_options(-Wno-deprecated-declarations)
add_compile_options(-Wno-unused-but-set-variable)

add_executable(
  karabiner_test
  src/test.cpp
  ../../../src/vendor/duktape-2.7.0/src/duktape.c
  ../../../src/vendor/duktape-2.7.0/extras/console/duk_console.c
  ../../../src/vendor/duktape-2.7.0/extras/module-node/duk_module_node.c
)
