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
  src/AppDelegate.m
  src/AppDelegate.h
  src/ComplexModificationsAssetsOutlineCellView.m
  src/ComplexModificationsAssetsOutlineCellView.h
  src/ComplexModificationsAssetsOutlineViewDataSource.m
  src/ComplexModificationsAssetsOutlineViewDataSource.h
  src/ComplexModificationsAssetsOutlineViewDelegate.m
  src/ComplexModificationsAssetsOutlineViewDelegate.h
  src/ComplexModificationsFileImportWindowController.m
  src/ComplexModificationsFileImportWindowController.h
  src/ComplexModificationsParametersTabController.m
  src/ComplexModificationsParametersTabController.h
  src/ComplexModificationsRulesTableCellView.m
  src/ComplexModificationsRulesTableCellView.h
  src/ComplexModificationsRulesTableViewController.m
  src/ComplexModificationsRulesTableViewController.h
  src/ComplexModificationsRulesTableViewDataSource.m
  src/ComplexModificationsRulesTableViewDataSource.h
  src/ComplexModificationsRulesTableViewDelegate.m
  src/ComplexModificationsRulesTableViewDelegate.h
  src/DevicesTableCellView.m
  src/DevicesTableCellView.h
  src/DevicesTableViewController.m
  src/DevicesTableViewController.h
  src/DevicesTableViewDataSource.m
  src/DevicesTableViewDataSource.h
  src/DevicesTableViewDelegate.m
  src/DevicesTableViewDelegate.h
  src/FnFunctionKeysTableViewController.m
  src/FnFunctionKeysTableViewController.h
  src/FnFunctionKeysTableViewDataSource.m
  src/FnFunctionKeysTableViewDataSource.h
  src/FnFunctionKeysTableViewDelegate.m
  src/FnFunctionKeysTableViewDelegate.h
  src/InputMonitoringAlertWindowController.m
  src/InputMonitoringAlertWindowController.h
  src/KextdAlertWindowController.m
  src/KextdAlertWindowController.h
  src/LogFileTextViewController.m
  src/LogFileTextViewController.h
  src/main.m
  src/PreferencesWindowController.m
  src/PreferencesWindowController.h
  src/ProfilesTableCellView.m
  src/ProfilesTableCellView.h
  src/ProfilesTableView.m
  src/ProfilesTableView.h
  src/ProfilesTableViewController.m
  src/ProfilesTableViewController.h
  src/ProfilesTableViewDataSource.m
  src/ProfilesTableViewDataSource.h
  src/ProfilesTableViewDelegate.m
  src/ProfilesTableViewDelegate.h
  src/SimpleModificationsMenuManager.m
  src/SimpleModificationsMenuManager.h
  src/SimpleModificationsTableCellView.m
  src/SimpleModificationsTableCellView.h
  src/SimpleModificationsTableViewController.m
  src/SimpleModificationsTableViewController.h
  src/SimpleModificationsTableViewDataSource.m
  src/SimpleModificationsTableViewDataSource.h
  src/SimpleModificationsTableViewDelegate.m
  src/SimpleModificationsTableViewDelegate.h
  src/SimpleModificationsTargetDeviceMenuManager.m
  src/SimpleModificationsTargetDeviceMenuManager.h
  src/SystemPreferencesManager.m
  src/SystemPreferencesManager.h
  src/SystemPreferencesModel.m
  src/SystemPreferencesModel.h
  src/VirtualHIDKeyboardTypeBackgroundView.m
  src/VirtualHIDKeyboardTypeBackgroundView.h
  Resources/Assets.xcassets
  Resources/input_monitoring.png
  Resources/kext_policy.png
  Resources/MainMenu.xib
  Resources/simple_modifications.json
  ../share/Resources/app.icns
)

set_source_files_properties(
  Resources/Assets.xcassets
  Resources/input_monitoring.png
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
