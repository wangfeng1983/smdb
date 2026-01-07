# Find GoogleTest
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Fallback: download and build GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Don't let gtest override C++ standard
    set(CMAKE_CXX_STANDARD_CACHE 21 CACHE STRING "C++ standard" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test executable
add_executable(memorydb_tests
    main.cpp
    test_database.cpp
    test_mem_manager.cpp
)

target_link_libraries(memorydb_tests
    PRIVATE
    memorydb
    gtest
    gtest_main
)

target_include_directories(memorydb_tests
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(memorydb_tests)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND memorydb_tests
    DEPENDS memorydb_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running all tests..."
)
