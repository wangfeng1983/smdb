# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE memorydb)
target_include_directories(basic_example PRIVATE ${PROJECT_SOURCE_DIR}/include)
