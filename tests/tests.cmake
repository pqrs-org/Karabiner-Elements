include (${CMAKE_CURRENT_LIST_DIR}/../src/common.cmake)

include_directories(${CMAKE_CURRENT_LIST_DIR}/vendor/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/test_runner/include)

link_directories(${CMAKE_CURRENT_LIST_DIR}/lib/test_runner/build)
