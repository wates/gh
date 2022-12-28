cmake_minimum_required(VERSION 2.6)

include_directories("../src")

add_executable(game3
  main.cc
)
target_link_libraries(game3 gh freetype json11 zlib)
