cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (Karabiner-MultitouchExtension)

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
  Karabiner-MultitouchExtension
  MACOSX_BUNDLE
  src/AppDelegate.h
  src/AppDelegate.m
  src/FingerStatus.h
  src/FingerStatus.m
  src/IgnoredAreaView.h
  src/IgnoredAreaView.m
  src/PreferencesController.h
  src/PreferencesController.m
  src/PreferencesKeys.h
  src/main.m
  Resources/MainMenu.xib
  ../share/Resources/app.icns
)

set_source_files_properties(
  Resources/MainMenu.xib
  ../share/Resources/app.icns
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(
  Karabiner-MultitouchExtension
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/Resources/Info.plist.in
)

target_link_libraries(
  Karabiner-MultitouchExtension
  KarabinerKit
  krbn
  -F/System/Library/PrivateFrameworks
  "-framework MultitouchSupport"
  "-framework SystemConfiguration"
  c++
)
