cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (Karabiner-Elements)

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
  Karabiner-Elements
  MACOSX_BUNDLE
  src/AlertWindowController.m
  src/AppDelegate.m
  src/ComplexModificationsAssetsOutlineCellView.m
  src/ComplexModificationsAssetsOutlineViewDataSource.m
  src/ComplexModificationsAssetsOutlineViewDelegate.m
  src/ComplexModificationsFileImportWindowController.m
  src/ComplexModificationsParametersTabController.m
  src/ComplexModificationsRulesTableCellView.m
  src/ComplexModificationsRulesTableViewController.m
  src/ComplexModificationsRulesTableViewDataSource.m
  src/ComplexModificationsRulesTableViewDelegate.m
  src/DevicesTableCellView.m
  src/DevicesTableViewController.m
  src/DevicesTableViewDataSource.m
  src/DevicesTableViewDelegate.m
  src/FnFunctionKeysTableViewController.m
  src/FnFunctionKeysTableViewDataSource.m
  src/FnFunctionKeysTableViewDelegate.m
  src/LogFileTextViewController.m
  src/main.m
  src/PreferencesWindowController.m
  src/ProfilesTableCellView.m
  src/ProfilesTableView.m
  src/ProfilesTableViewController.m
  src/ProfilesTableViewDataSource.m
  src/ProfilesTableViewDelegate.m
  src/SimpleModificationsMenuManager.m
  src/SimpleModificationsTableCellView.m
  src/SimpleModificationsTableViewController.m
  src/SimpleModificationsTableViewDataSource.m
  src/SimpleModificationsTableViewDelegate.m
  src/SimpleModificationsTargetDeviceMenuManager.m
  src/SystemPreferencesManager.m
  src/SystemPreferencesModel.m
  src/VirtualHIDKeyboardTypeBackgroundView.m
  Resources/Assets.xcassets
  Resources/kext_policy.png
  Resources/MainMenu.xib
  Resources/simple_modifications.json
  ../share/Resources/app.icns
)

set_source_files_properties(
  Resources/Assets.xcassets
  Resources/kext_policy.png
  Resources/MainMenu.xib
  Resources/simple_modifications.json
  ../share/Resources/app.icns
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(
  Karabiner-Elements
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/Resources/Info.plist.in
)

target_link_libraries(
  Karabiner-Elements
  KarabinerKit
  krbn
  "-framework SystemConfiguration"
  c++
)
