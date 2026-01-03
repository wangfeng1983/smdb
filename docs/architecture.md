# SMDB Architecture Overview

## Design Philosophy

SMDB (Simple Memory Database) is a modern C++21 implementation of an in-memory database system, inspired by MDB's proven architecture. The design emphasizes:

- **Modern C++21**: Utilizing latest C++ features (concepts, ranges, std::expected, etc.)
- **Type Safety**: Strong typing with compile-time guarantees
- **Zero-Cost Abstractions**: Minimal runtime overhead
- **Clear Separation of Concerns**: Modular architecture with well-defined interfaces
- **Testability**: Interface-based design for easy unit testing

## Architecture Layers

```
┌─────────────────────────────────────────────────────┐
│                    Application Layer                │
│              (Console, Tools, Client Library)        │
├─────────────────────────────────────────────────────┤
│                     API Layer                       │
│           (Database, Table, Transaction APIs)        │
├─────────────────────────────────────────────────────┤
│                   Core Business Layer               │
│  ┌──────────┬──────────┬──────────┬──────────────┐  │
│  │  Table   │  Index   │ Transaction │   Schema   │  │
│  │ Manager  │ Manager  │   Manager  │  Manager   │  │
│  └──────────┴──────────┴──────────┴──────────────┘  │
├─────────────────────────────────────────────────────┤
│                  Storage Engine Layer               │
│  ┌──────────┬──────────┬──────────┬──────────────┐  │
│  │   Mem    │   Lock   │   Latch  │    Buffer    │  │
│  │ Manager  │ Manager  │ Manager  │    Manager   │  │
│  └──────────┴──────────┴──────────┴──────────────┘  │
├─────────────────────────────────────────────────────┤
│                  Recovery Layer                     │
│  ┌──────────┬──────────┬──────────────────────────┐ │
│  │ Redo Log │ Undo Log │   Recovery Manager       │ │
│  └──────────┴──────────┴──────────────────────────┘ │
├─────────────────────────────────────────────────────┤
│                   Persistence Layer                 │
│              (File System, Shared Memory)           │
└─────────────────────────────────────────────────────┘
```

## Core Modules

### 1. Core Module (`core/`)

**Database** (`database.h`)
- Central entry point for all database operations
- Manages database lifecycle (initialize, start, stop, shutdown)
- Coordinates all other modules
- Provides table and transaction management
- Handles checkpoint triggering

**Table** (`table.h`)
- Represents a database table
- Provides CRUD operations (Create, Read, Update, Delete)
- Manages table schema and metadata
- Coordinates index operations
- Integrates with transaction management

### 2. Storage Module (`storage/`)

**Memory Manager** (`mem_manager.h`)
- Page-based memory allocation (8KB pages)
- Slot management within pages
- Address translation (virtual to physical)
- Dirty page tracking
- Memory statistics and monitoring
- Checkpoint support (flush dirty pages)

Key innovations:
- Space-based segregation (Control, Table, Index, Undo, Redo)
- Efficient allocation with free lists
- LRU-friendly page organization

### 3. Index Module (`index/`)

**Index** (`index.h`)
- Abstract index interface
- Multiple index types: Hash, B-Tree, Bitmap
- Composite key support
- Unique constraint enforcement
- Index statistics and maintenance

Index types:
- **Hash Index**: O(1) exact match queries
- **B-Tree Index**: O(log n) range queries and ordering
- **Bitmap Index**: Multi-column boolean queries

### 4. Transaction Module (`transaction/`)

**Transaction** (`transaction.h`)
- ACID transaction support
- Savepoint support
- Transaction isolation levels
- Operation logging for undo/redo
- Transaction lifecycle management

**Transaction Manager** (`transaction_mgr.h`)
- Manages all active transactions
- Transaction ID generation
- Deadlock detection coordination
- Transaction cleanup

### 5. Concurrency Module (`concurrency/`)

**Lock Manager** (`lock_manager.h`)
- Multi-granularity locking (table, page, row)
- Lock compatibility matrix
- Deadlock detection (wait-for graph)
- Lock escalation
- Timeouts and cancellation

**Latch Manager** (`lock_manager.h`)
- Lightweight in-memory synchronization
- Reader-writer locks
- Spinlocks for hot paths
- Used for internal data structures

Lock types:
- **Shared (S)**: Read lock
- **Exclusive (X)**: Write lock
- **Intention Shared (IS)**: Intention to read
- **Intention Exclusive (IX)**: Intention to write

### 6. Recovery Module (`recovery/`)

**Recovery Manager** (`recovery_manager.h`)
- ARIES-style recovery algorithm
- Three phases: Analysis, Redo, Undo
- Checkpoint management
- Crash recovery

**Redo Log** (`recovery_manager.h`)
- Write-Ahead Logging (WAL)
- Sequential log writes
- Log buffering and batching
- Log truncation

**Undo Log** (`recovery_manager.h`)
- In-memory undo records
- Transaction rollback support
- MVCC version management

## Key Design Decisions

### 1. Modern C++21 Features

**std::expected<T, E>**
- Type-safe error handling
- No exceptions for control flow
- Explicit error propagation

**Concepts**
- Compile-time type checking
- Clear template constraints
- Better error messages

**std::variant**
- Type-safe unions
- Sum types for Values
- Pattern matching support

**std::string_view**
- Zero-copy string operations
- Reduced allocations
- API efficiency

### 2. Memory Management

**Page-Based Allocation**
- Fixed 8KB pages (similar to filesystem)
- Efficient buddy allocator
- Reduced fragmentation
- Cache-friendly access patterns

**Space Segregation**
- Separate spaces for different data types
- Independent allocation strategies
- Targeted optimization

**Address Translation**
- Virtual to physical address mapping
- Shared memory support
- Process transparency

### 3. Transaction Model

**SCN-Based Versioning**
- Monotonically increasing System Change Number
- Snapshot isolation
- MVCC support
- Point-in-time queries

**Undo/Redo Logging**
- Before and after images
- Atomic operations
- Crash recovery
- Rollback support

### 4. Indexing Strategy

**Pluggable Index Types**
- Strategy pattern
- Runtime index selection
- Type-specific optimization

**Composite Keys**
- Multi-column indexes
- Flexible key generation
- Covering indexes

## Data Flow Examples

### 1. Insert Operation

```
User Request
    ↓
Database.beginTransaction()
    ↓
Transaction.begin()
    ↓
Table.insert(row, tx)
    ↓
LockManager.acquireLock(row_id, Exclusive)
    ↓
MemManager.allocateSlot(page_id, size)
    ↓
RedoLog.write(entry)
    ↓
Index.insert(key, row_id)
    ↓
Transaction.commit()
    ↓
LockManager.releaseAllLocks(tx_id)
    ↓
RedoLog.flush()
```

### 2. Query Operation

```
User Request
    ↓
Database.beginTransaction()
    ↓
Table.scan(predicate, tx)
    ↓
LockManager.acquireLock(table, Shared)
    ↓
Index.lookup(key) [if indexed]
    ↓
MemManager.getSlot(page_id, slot_id)
    ↓
Filter rows (predicate)
    ↓
LockManager.releaseLock(table)
    ↓
Transaction.commit()
```

### 3. Crash Recovery

```
Database.start()
    ↓
RecoveryManager.recover()
    ↓
┌─────────────┐
│ Analysis    │ → Build transaction table
│   Phase     │ → Find last checkpoint
└─────────────┘
    ↓
┌─────────────┐
│ Redo Phase  │ → Replay committed transactions
│             │ → Restore to crash state
└─────────────┘
    ↓
┌─────────────┐
│ Undo Phase  │ → Rollback uncommitted transactions
│             │ → Restore consistency
└─────────────┘
    ↓
Database ready
```

## Performance Optimizations

1. **Lock-Free Reads**: MVCC allows reads without locks
2. **Lazy Updates**: Batch writes to reduce I/O
3. **Index Selection**: Query optimizer chooses best index
4. **Buffer Pool**: Keep hot pages in memory
5. **Log Buffering**: Group log writes
6. **Checkpoint**: Reduce recovery time

## Testing Strategy

- **Unit Tests**: Test each module in isolation
- **Integration Tests**: Test module interactions
- **Recovery Tests**: Crash simulation and recovery
- **Concurrency Tests**: Multi-threaded stress tests
- **Performance Tests**: Benchmark key operations

## Roadmap

### Phase 1: Core Infrastructure (Current)
- [x] Type definitions
- [x] Interface definitions
- [ ] Memory manager implementation
- [ ] Lock manager implementation
- [ ] Transaction manager implementation

### Phase 2: Storage & Indexing
- [ ] Table implementation
- [ ] B-Tree index implementation
- [ ] Hash index implementation
- [ ] Bitmap index implementation

### Phase 3: Transaction & Recovery
- [ ] Redo log implementation
- [ ] Undo log implementation
- [ ] Recovery manager implementation
- [ ] Checkpoint implementation

### Phase 4: Database Core
- [ ] Database implementation
- [ ] Query processing
- [ ] SQL parser (optional)

### Phase 5: Tools & Utilities
- [ ] Command-line tool
- [ ] Dump/load utilities
- [ ] Monitoring tools
- [ ] Benchmark suite

## References

1. **MDB Source Code**: Original implementation
2. **ARIES Algorithm**: IBM's recovery algorithm
3. **MySQL Architecture**: Buffer pool and locking
4. **PostgreSQL MVCC**: Multi-version concurrency control
5. **LMDB Design**: Memory-mapped database

## Conclusion

SMDB brings MDB's proven architecture to modern C++21, providing a solid foundation for an in-memory database system with enterprise-grade features like ACID transactions, crash recovery, and multiple index types.
