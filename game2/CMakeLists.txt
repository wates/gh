cmake_minimum_required(VERSION 2.6)

include_directories("../src")

add_executable(game2
  main.cc
  scene.h
  scene.cc
)
target_link_libraries(game2 gh freetype json11 zlib)
