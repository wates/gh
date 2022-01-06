cmake_minimum_required(VERSION 2.6)

include_directories("../src")

add_executable(game
  main.cc
  tuid.h
  tuid.cc
  doc.h
  doc.cc
  scene.h
  scene.cc
)
target_link_libraries(game gh freetype json11 zlib)
