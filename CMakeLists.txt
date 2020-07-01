cmake_minimum_required(VERSION 3.2)

project(gh)

if(MSVC)
set(CompilerFlags
CMAKE_CXX_FLAGS
CMAKE_CXX_FLAGS_DEBUG
CMAKE_CXX_FLAGS_RELEASE
CMAKE_C_FLAGS
CMAKE_C_FLAGS_DEBUG
CMAKE_C_FLAGS_RELEASE
)
if (MSVC_VERSION GREATER_EQUAL "1900")
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
  if (_cpp_latest_flag_supported)
    add_compile_options("/std:c++latest")
  endif()
endif()

foreach(CompilerFlag ${CompilerFlags})
string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()
#  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
#  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
  include_directories("$ENV{GLESSDK_DIR}/include" "$ENV{DXSDK_DIR}/include")
  link_directories($ENV{OGLES2_DIR}/WindowsX64/Lib $ENV{DXSDK_DIR}/lib/x64)
  find_library(EGL libEGL "$ENV{GLESLIB_DIR}")
  find_library(GLESv2 libGLESv2 "$ENV{GLESLIB_DIR}")
  add_definitions(-D_UNICODE -DUNICODE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /wd4819")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4819")
endif(MSVC)
if(ANDROID)
endif(ANDROID)

add_subdirectory(third)
include_directories(third/json11)
include_directories(third/zlib/include)
include_directories(third/freetype/include)

add_subdirectory(src)
if(MSVC)
  add_subdirectory(test)
endif()
