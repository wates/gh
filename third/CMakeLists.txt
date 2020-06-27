cmake_minimum_required(VERSION 2.6)

add_subdirectory(json11)
include_directories(json11)
add_subdirectory(zlib)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/zlib/include")
add_subdirectory(freetype)
