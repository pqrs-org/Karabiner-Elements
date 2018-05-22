cmake_minimum_required (VERSION 3.9)

include (../src/common.cmake)

configure_file (
  Distribution.xml.in
  Distribution.xml
)
