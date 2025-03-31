cmake_minimum_required(VERSION 3.27)
project(smdb)

set(CMAKE_CXX_STANDARD 23)
add_subdirectory(libs)

add_executable(smdb main.cpp)


