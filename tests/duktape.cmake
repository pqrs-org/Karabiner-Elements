set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_CXX_STANDARD 20)

include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-src)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/console)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-2.7.0/extras/module-node)

add_library(libduktape STATIC IMPORTED)
set_target_properties(libduktape PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/../vendor/duktape-src/build/Release/libduktape.a)
