cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (Karabiner-EventViewer)

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
  Karabiner-EventViewer
  MACOSX_BUNDLE
  src/AppDelegate.m
  src/DevicesController.m
  src/EventQueue.m
  src/FrontmostApplicationController.m
  src/KeyResponder.m
  src/main.m
  src/PreferencesManager.m
  src/TabView.m
  src/VariablesController.m
  Resources/MainMenu.xib
  Resources/app.icns
)

set_source_files_properties(
  Resources/MainMenu.xib
  Resources/app.icns
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(
  Karabiner-EventViewer
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/Resources/Info.plist.in
)

target_link_libraries(
  Karabiner-EventViewer
  KarabinerKit
  krbn
  "-framework SystemConfiguration"
  c++
)
