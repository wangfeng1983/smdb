# SMDB - Simple Memory Database

A high-performance in-memory database system implemented in modern C++21, inspired by MDB's proven architecture.

## Features

- **Modern C++21**: Utilizing latest C++ features (concepts, ranges, std::expected, etc.)
- **ACID Transactions**: Full transaction support with begin, commit, rollback, and savepoints
- **Multiple Index Types**: Hash, B-Tree, and Bitmap indexes
- **Crash Recovery**: ARIES-style recovery with redo/undo logging
- **Page-Based Storage**: 8KB page-based memory management
- **Multi-Version Concurrency Control (MVCC)**: Snapshot isolation
- **Multi-Granularity Locking**: Table, page, and row-level locks with deadlock detection
- **Type-Safe API**: Modern C++ interfaces with std::expected for error handling
- **Comprehensive Testing**: Unit tests and integration tests
- **Cross-Platform**: Support for Windows, Linux, and macOS

## Architecture

SMDB follows a layered architecture design:

```
Application Layer
    ↓
API Layer (Database, Table, Transaction)
    ↓
Core Business Layer (Table, Index, Transaction Managers)
    ↓
Storage Engine (Memory, Lock, Latch Managers)
    ↓
Recovery Layer (Redo/Undo Logs, Recovery Manager)
    ↓
Persistence Layer (File System, Shared Memory)
```

For detailed architecture information, see [docs/architecture.md](docs/architecture.md).

## Project Structure

```
smdb/
├── CMakeLists.txt              # Main CMake configuration
├── README.md                   # This file
├── build.sh                    # Unix/Linux build script
├── build.bat                   # Windows build script
├── include/smdb/               # Public headers
│   ├── core/                   # Core interfaces
│   │   ├── database.h          # Database interface
│   │   └── table.h             # Table interface
│   ├── storage/                # Storage layer
│   │   └── mem_manager.h       # Memory manager interface
│   ├── index/                  # Index system
│   │   └── index.h             # Index interface
│   ├── transaction/            # Transaction management
│   │   └── transaction.h       # Transaction interface
│   ├── concurrency/            # Concurrency control
│   │   └── lock_manager.h      # Lock manager interface
│   ├── recovery/               # Recovery system
│   │   └── recovery_manager.h  # Recovery manager interface
│   └── utils/                  # Utilities
│       └── types.h             # Type definitions
├── src/                        # Source files
│   ├── core/                   # Core implementations
│   ├── storage/                # Storage implementations
│   ├── transaction/            # Transaction implementations
│   ├── concurrency/            # Concurrency implementations
│   ├── index/                  # Index implementations
│   ├── recovery/               # Recovery implementations
│   └── utils/                  # Utility implementations
├── tests/                      # Unit tests
│   ├── unit/                   # Unit tests
│   └── integration/            # Integration tests
├── examples/                   # Example programs
└── docs/                       # Documentation
    └── architecture.md         # Architecture documentation
```

## Requirements

- CMake 3.20 or higher
- C++21 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- GoogleTest (automatically downloaded by CMake)

## Building

### Linux / macOS

```bash
# Using the build script
chmod +x build.sh
./build.sh

# Or manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows

```cmd
# Using the build script
build.bat

# Or manually
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Build Options

```bash
# Build with tests (default)
cmake .. -DBUILD_TESTS=ON

# Build without tests
cmake .. -DBUILD_TESTS=OFF

# Build with sanitizers
cmake .. -DENABLE_SANITIZERS=ON

# Build with code coverage
cmake .. -DENABLE_COVERAGE=ON

# Build shared library
cmake .. -DBUILD_SHARED_LIBS=ON
```

## Running Tests

```bash
cd build
ctest --output-on-failure

# Or directly
./bin/memorydb_tests
```

## Usage Example

```cpp
#include "memorydb/memorydb.h"
#include <iostream>

int main() {
    // Create database instance
    auto db = memorydb::createDatabase();

    // Basic operations
    db->put("name", std::string("MemoryDB"));
    db->put("version", 1);
    db->put("performance", 99.9);

    memorydb::Value value;
    db->get("name", value);
    std::cout << "Name: " << std::get<std::string>(value) << std::endl;

    // Transaction support
    db->beginTransaction();
    db->put("balance", 1000);
    db->commitTransaction();

    // Check existence
    if (db->exists("name")) {
        std::cout << "Key exists!" << std::endl;
    }

    // Delete
    db->remove("version");
    std::cout << "Database size: " << db->size() << std::endl;

    return 0;
}
```

## Running Examples

```bash
cd build
./bin/basic_example
```

## API Reference

### Database Interface

| Method | Description |
|--------|-------------|
| `put(key, value)` | Insert or update a key-value pair |
| `get(key, value)` | Retrieve value by key |
| `remove(key)` | Delete a key-value pair |
| `exists(key)` | Check if a key exists |
| `clear()` | Clear all data |
| `size()` | Get number of keys |
| `beginTransaction()` | Begin a transaction |
| `commitTransaction()` | Commit transaction changes |
| `rollbackTransaction()` | Rollback transaction changes |

## License

This project is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Roadmap

- [ ] Thread-safe concurrent operations
- [ ] Persistence to disk
- [ ] Query operations (range scans, filters)
- [ ] Index support
- [ ] TTL (Time-To-Live) for keys
- [ ] MVCC (Multi-Version Concurrency Control)
- [ ] Replication support
