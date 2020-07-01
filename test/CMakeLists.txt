cmake_minimum_required(VERSION 2.6)

project(gh_tests)

include_directories(.)

add_subdirectory(googletest)

add_executable(test_viewport viewport.cc)
target_link_libraries(test_viewport gh gtest gtest_main)
add_executable(test_gfx gfx.cc)
target_link_libraries(test_gfx gh gtest gtest_main freetype)
add_executable(test_sys sys.cc)
target_link_libraries(test_sys gh gtest gtest_main)

