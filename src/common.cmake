# Common configurations

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

set(CMAKE_CXX_STANDARD 17)

include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/vendor)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/vendor/cget/include)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/vendor/Karabiner-DriverKit-VirtualHIDDevice/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/share)

add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-O2)
add_compile_options(-fobjc-arc)
add_compile_options(-fmodules)

add_definitions(-DFMT_HEADER_ONLY)

# Version

file(READ ${CMAKE_CURRENT_LIST_DIR}/../version KARABINER_VERSION)
string(STRIP ${KARABINER_VERSION} KARABINER_VERSION)

configure_file (
  ${CMAKE_CURRENT_LIST_DIR}/share/karabiner_version.h.in
  ${CMAKE_CURRENT_LIST_DIR}/share/karabiner_version.h
)

# Variables to avoid string replacement in Info.plist

set(PLIST_EXECUTABLE_NAME "$\{EXECUTABLE_NAME\}")
set(PLIST_MACOSX_DEPLOYMENT_TARGET "$\{MACOSX_DEPLOYMENT_TARGET\}")
set(PLIST_PRODUCT_NAME "$\{PRODUCT_NAME\}")
