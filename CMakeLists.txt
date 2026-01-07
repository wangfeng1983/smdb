cmake_minimum_required(VERSION 3.15)
project(MemoryDB VERSION 0.1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)  # Use C++20 for broader compatibility
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
    # Temporarily disable warnings as errors for development
    # add_compile_options(/WX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)  # Disable CRT deprecation warnings
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /GL)
        add_link_options(/LTCG)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Temporarily disable warnings as errors for development
    # add_compile_options(-Werror)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native)
    endif()
endif()

# Sanitizers
if(ENABLE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# Code coverage
if(ENABLE_COVERAGE AND NOT MSVC)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Set C++ standard modules path (if using modules)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Print configuration summary
message(STATUS "")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "  MemoryDB Configuration Summary")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "  Version:             ${PROJECT_VERSION}")
message(STATUS "  Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:        ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests:         ${BUILD_TESTS}")
message(STATUS "  Build examples:      ${BUILD_EXAMPLES}")
message(STATUS "  Build shared libs:   ${BUILD_SHARED_LIBS}")
message(STATUS "  Enable sanitizers:   ${ENABLE_SANITIZERS}")
message(STATUS "  Enable coverage:     ${ENABLE_COVERAGE}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "")
