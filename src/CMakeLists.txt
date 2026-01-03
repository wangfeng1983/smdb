# Collect all source files for each module
set(CORE_SOURCES)
set(STORAGE_SOURCES)
set(TRANSACTION_SOURCES)
set(CONCURRENCY_SOURCES)
set(INDEX_SOURCES)
set(RECOVERY_SOURCES)
set(UTILS_SOURCES)

# Core module
file(GLOB CORE_SOURCES
    "core/*.cpp"
)

# Storage module
file(GLOB STORAGE_SOURCES
    "storage/*.cpp"
)

# Transaction module
file(GLOB TRANSACTION_SOURCES
    "transaction/*.cpp"
)

# Concurrency module
file(GLOB CONCURRENCY_SOURCES
    "concurrency/*.cpp"
)

# Index module
file(GLOB INDEX_SOURCES
    "index/*.cpp"
)

# Recovery module
file(GLOB RECOVERY_SOURCES
    "recovery/*.cpp"
)

# Utils module
file(GLOB UTILS_SOURCES
    "utils/*.cpp"
)

# Collect all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${STORAGE_SOURCES}
    ${TRANSACTION_SOURCES}
    ${CONCURRENCY_SOURCES}
    ${INDEX_SOURCES}
    ${RECOVERY_SOURCES}
    ${UTILS_SOURCES}
)

# Also collect old memorydb files for backward compatibility
file(GLOB OLD_SOURCES "*.cpp")
list(APPEND ALL_SOURCES ${OLD_SOURCES})

# Collect all headers
file(GLOB_RECURSE HEADERS "${PROJECT_SOURCE_DIR}/include/*.h" "${PROJECT_SOURCE_DIR}/include/*.hpp")

# Create library
add_library(smdb ${ALL_SOURCES})
add_library(memorydb::memorydb ALIAS smdb)

# Set library version
set_target_properties(smdb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${HEADERS}"
    OUTPUT_NAME "smdb"
)

# Link pthread on Unix systems
if(UNIX AND NOT APPLE)
    target_link_libraries(smdb PRIVATE pthread)
elseif(APPLE)
    # macOS doesn't need explicit pthread linking
endif()

# Installation
install(TARGETS smdb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
