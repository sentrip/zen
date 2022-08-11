cmake_minimum_required(VERSION 3.17)

project(zen)

set(CMAKE_CXX_STANDARD 20)

add_library(zen INTERFACE)
target_include_directories(zen INTERFACE src)

add_subdirectory(bench)
add_subdirectory(test)
