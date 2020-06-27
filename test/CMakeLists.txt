cmake_minimum_required(VERSION 2.6)

project(gh_tests)

include_directories(.)

add_subdirectory(googletest)

add_executable(test_viewport viewport.cc)

target_link_libraries(test_viewport gh gtest gtest_main)

