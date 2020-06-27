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
foreach(CompilerFlag ${CompilerFlags})
string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()
#  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
#  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
  add_definitions(-D_UNICODE -DUNICODE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /wd4819")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4819")
endif(MSVC)
if(ANDROID)
endif(ANDROID)

add_subdirectory(third)
add_subdirectory(src)
if(MSVC)
  add_subdirectory(test)
endif()
