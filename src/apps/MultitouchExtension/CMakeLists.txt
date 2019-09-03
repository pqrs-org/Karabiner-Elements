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
  src/FingerStatusEntry.h
  src/FingerStatusEntry.m
  src/FingerStatusManager.h
  src/FingerStatusManager.m
  src/IgnoredAreaView.h
  src/IgnoredAreaView.m
  src/main.m
  src/MultitouchDeviceManager.h
  src/MultitouchDeviceManager.m
  src/MultitouchPrivate.h
  src/PreferencesController.h
  src/PreferencesController.m
  src/PreferencesKeys.h
  Resources/MainMenu.xib
  Resources/app.icns
  Resources/Assets.xcassets
)

set_source_files_properties(
  Resources/MainMenu.xib
  Resources/app.icns
  Resources/Assets.xcassets
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
