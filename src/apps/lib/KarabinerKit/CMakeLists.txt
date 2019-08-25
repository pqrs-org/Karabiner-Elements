cmake_minimum_required (VERSION 3.9)

include (../../../common.cmake)

project (KarabinerKit)

include_directories(
  ../../../lib/libkrbn/include
  ./include
)

add_library(
  KarabinerKit
  include/KarabinerKit/ComplexModificationsAssetsFileModel.h
  include/KarabinerKit/ComplexModificationsAssetsManager.h
  include/KarabinerKit/ComplexModificationsAssetsRuleModel.h
  include/KarabinerKit/ConfigurationManager.h
  include/KarabinerKit/ConnectedDevices.h
  include/KarabinerKit/CoreConfigurationModel.h
  include/KarabinerKit/DeviceManager.h
  include/KarabinerKit/JsonUtility.h
  include/KarabinerKit/KarabinerKit.h
  include/KarabinerKit/NotificationKeys.h
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
