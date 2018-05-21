cmake_minimum_required (VERSION 3.9)

include (../../../common.cmake)

project (KarabinerKit)

add_compile_options(
  -Wall
  -Werror
  -O2
  -fmodules
  -fobjc-arc
)

include_directories(
  /opt/local/include
  /usr/local/include
  ../../../share
  ../../../vendor
  ../../../lib/libkrbn
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

target_link_libraries(
  KarabinerKit
  ../../../lib/libkrbn/build/libkrbn.a
)
