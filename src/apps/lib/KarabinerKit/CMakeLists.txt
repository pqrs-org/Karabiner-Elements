cmake_minimum_required (VERSION 3.9)

include (../../../common.cmake)

project (KarabinerKit)

include_directories(
  ../../../lib/libkrbn/include
  ./include
)

add_library(
  KarabinerKit
  src/ComplexModificationsAssetsFileModel.m
  src/ComplexModificationsAssetsManager.m
  src/ComplexModificationsAssetsRuleModel.m
  src/ConfigurationManager.m
  src/ConnectedDevices.m
  src/CoreConfigurationModel.m
  src/DeviceManager.m
  src/JsonUtility.m
  src/KarabinerKit.m
)
