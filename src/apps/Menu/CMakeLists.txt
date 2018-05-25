cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (Karabiner-Menu)

include_directories(
  ../../lib/libkrbn/include
  ../lib/KarabinerKit/include
  ../share
)

link_directories(
  ../../lib/libkrbn/build_xcode/build
  ../lib/KarabinerKit/build_xcode/build
)

add_executable(
  Karabiner-Menu
  MACOSX_BUNDLE
  src/AppDelegate.m
  src/main.m
  src/MenuController.m
  Resources/Assets.xcassets
  Resources/MainMenu.xib
  ../share/Resources/app.icns
)

set_source_files_properties(
  Resources/Assets.xcassets
  Resources/MainMenu.xib
  ../share/Resources/app.icns
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(
  Karabiner-Menu
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/Resources/plist.in
)

target_link_libraries(
  Karabiner-Menu
  KarabinerKit
  krbn
  "-framework SystemConfiguration"
  c++
)
