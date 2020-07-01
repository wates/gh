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
  main/md5.h
  main/md5.cc
  sys/viewport.cc
  sys/viewport.h
  sys/viewport_windows.cc
  sys/fs.h
  sys/fs.cc
  sys/fs_windows.cc
  gfx/graphics.h
  gfx/graphics_d3d9.h
  gfx/graphics_d3d9.cc
  gfx/color.h
  gfx/color.cc
  gfx/fertex.h
  gfx/fertex.cc
  gfx/geometory.h
  gfx/geometory.cc
  gfx/matrix.cc
  gfx/matrix.cc
  gfx/shader.h
  gfx/shader.cc
  gfx/shader_hlsl.h
  gfx/shader_hlsl.cc
  gfx/shape.h
  gfx/shape.cc
  gfx/fontmap.h
  gfx/fontmap.cc
)