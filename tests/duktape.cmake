set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-src)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/console)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/module-node)

set(DUKTAPE_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/build/libduktape_lib.a)

if (EXISTS ${DUKTAPE_LIB_PATH})
  add_library(duktape_lib STATIC IMPORTED)
  set_target_properties(duktape_lib PROPERTIES IMPORTED_LOCATION ${DUKTAPE_LIB_PATH})
else ()
  add_library(duktape_lib STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-src/duktape.c
    ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/console/duk_console.c
    ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/module-node/duk_module_node.c
  )

  set_target_properties(duktape_lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build)

  # Suppress duktape warnings for duktape_lib
  target_compile_options(
    duktape_lib
    PRIVATE
      -Wno-deprecated-declarations
      -Wno-unused-but-set-variable
  )
endif ()
