cmake_minimum_required (VERSION 3.9)

include (../../common.cmake)

project (karabiner_observer)

include_directories(
  ./include
)

add_executable(
  karabiner_observer
  src/main.cpp
)

target_link_libraries(
  karabiner_observer
  "-framework CoreFoundation"
  "-framework CoreServices"
  "-framework IOKit"
)
