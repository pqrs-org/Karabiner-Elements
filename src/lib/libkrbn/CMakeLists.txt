cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (krbn)

add_compile_options(
  -Wall
  -Werror
  -O2
  -std=c++14
)

include_directories(
  /opt/local/include
  /usr/local/include
  ../../share
  ../../vendor
)

add_library(
  krbn
  libkrbn.cpp
  libkrbn_complex_modifications_assets_manager.cpp
  libkrbn_configuration.cpp
  libkrbn_connected_devices.cpp
  libkrbn_file_monitor.cpp
  libkrbn_hid_value_observer.cpp
  libkrbn_log_monitor.cpp
  libkrbn_system_preferences_monitor.cpp
  libkrbn_version_monitor.cpp
)
