cmake_minimum_required(VERSION 3.27)
project(smdb)

if(NOT DEFINED SMDB_SOURCE_DIR)
    set(SMDB_SOURCE_DIR ${PROJECT_SOURCE_DIR})
endif()

set(CMAKE_CXX_STANDARD 23)
#add_subdirectory(libs)

add_executable(smdb main.cpp)


