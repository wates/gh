cmake_minimum_required(VERSION 2.6)

add_subdirectory(sys)
add_subdirectory(main)
add_subdirectory(gfx)


add_library(gh
  main/pipe.cc
  main/pipe.h
  main/underscore.h
  main/observer.cc
  main/observer.h
  main/msgq.cc
  main/msgq.h
  sys/viewport.cc
  sys/viewport.h
  sys/viewport_windows.cc
)