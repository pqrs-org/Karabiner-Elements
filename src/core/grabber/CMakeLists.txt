cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_grabber)

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
  ./include
)

find_library(CARBON_LIBRARY Carbon)
find_library(IOKIT_LIBRARY IOKit)
find_library(SYSTEMCONFIGURATION_LIBRARY SystemConfiguration)

add_executable(karabiner_grabber main.cpp)

target_link_libraries(
  karabiner_grabber
  ${CARBON_LIBRARY}
  ${IOKIT_LIBRARY}
  ${SYSTEMCONFIGURATION_LIBRARY}
)
