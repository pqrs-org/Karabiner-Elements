cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (Karabiner-NotificationWindow)

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
  Karabiner-NotificationWindow
  MACOSX_BUNDLE
  src/AppDelegate.m
  src/AppDelegate.h
  src/main.m
  src/NotificationWindowManager.m
  src/NotificationWindowManager.h
  src/NotificationWindowView.m
  src/NotificationWindowView.h
  Resources/MainMenu.xib
  Resources/NotificationWindow.xib
  Resources/Assets.xcassets
  ../share/Resources/app.icns
)

set_source_files_properties(
  Resources/Assets.xcassets
  Resources/MainMenu.xib
  Resources/NotificationWindow.xib
  ../share/Resources/app.icns
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(
  Karabiner-NotificationWindow
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/Resources/plist.in
)

target_link_libraries(
  Karabiner-NotificationWindow
  KarabinerKit
  krbn
  "-framework SystemConfiguration"
  c++
)
