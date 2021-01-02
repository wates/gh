cmake_minimum_required(VERSION 2.6)

include_directories("../src")

add_executable(game
  main.cc
)
target_link_libraries(game gh freetype)
