# Common configurations

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Version

file(READ ${CMAKE_CURRENT_LIST_DIR}/../version KARABINER_VERSION)
string(STRIP ${KARABINER_VERSION} KARABINER_VERSION)

configure_file (
  "${CMAKE_CURRENT_LIST_DIR}/share/karabiner_version.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/share/karabiner_version.h"
)

# Variables to avoid string replacement in Info.plist

set(PLIST_EXECUTABLE_NAME "$\{EXECUTABLE_NAME\}")
set(PLIST_MACOSX_DEPLOYMENT_TARGET "$\{MACOSX_DEPLOYMENT_TARGET\}")
set(PLIST_PRODUCT_NAME "$\{PRODUCT_NAME\}")
