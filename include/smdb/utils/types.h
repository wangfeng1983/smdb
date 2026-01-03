#ifndef SMDB_UTILS_TYPES_H
#define SMDB_UTILS_TYPES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <memory>
#include <chrono>
#include <concepts>
#include <expected>

namespace smdb {

// Forward declarations
class Database;
class Table;
class Index;
class Transaction;

// ===== Core Type Aliases =====
using DbName = std::string;
using TableName = std::string;
using IndexName = std::string;
using ColumnName = std::string;

// SCN (System Change Number) - 64-bit timestamp
using SCN = uint64_t;

// Transaction ID
using TransactionId = uint64_t;

// Row ID
using RowId = uint64_t;

// Page ID
using PageId = uint64_t;

// Slot ID
using SlotId = uint32_t;

// ===== Value Types =====
enum class ValueType : uint8_t {
    Null = 0,
    Int32,
    Int64,
    Float,
    Double,
    String,
    Boolean,
    Blob,
    Timestamp
};

using Value = std::variant<
    std::monostate,      // Null
    int32_t,             // Int32
    int64_t,             // Int64
    float,               // Float
    double,              // Double
    std::string,         // String
    bool,                // Boolean
    std::vector<uint8_t>,// Blob
    std::chrono::system_clock::time_point // Timestamp
>;

// ===== Result Type =====
template<typename T>
using Result = std::expected<T, std::string>;

// ===== Error Handling =====
struct Error {
    std::string message;
    int code;

    Error(std::string msg, int c = 0) : message(std::move(msg)), code(c) {}
};

// ===== Operation Types =====
enum class OperationType : uint8_t {
    DDL = 1,
    BeginTx = 2,
    CommitTx = 3,
    RollbackTx = 4,
    Insert = 5,
    Update = 6,
    Delete = 7,
    CreateIndex = 8,
    DropIndex = 9,
    Checkpoint = 10
};

// ===== Index Types =====
enum class IndexType : uint8_t {
    Hash = 0,
    BTree = 1,
    HashTree = 2,
    Bitmap = 3
};

// ===== Column Definition =====
struct ColumnDef {
    ColumnName name;
    ValueType type;
    size_t length = 0;        // For string/blob types
    bool nullable = true;
    bool primary_key = false;
    std::string default_value;

    bool operator==(const ColumnDef& other) const {
        return name == other.name && type == other.type;
    }
};

// ===== Transaction States =====
enum class TransactionState : uint8_t {
    Active = 0,
    Committing = 1,
    Committed = 2,
    RollingBack = 3,
    RolledBack = 4
};

// ===== Lock Types =====
enum class LockType : uint8_t {
    None = 0,
    Shared = 1,      // Read lock
    Exclusive = 2,   // Write lock
    IntentionShared = 3,
    IntentionExclusive = 4
};

// ===== Lock Duration =====
enum class LockDuration : uint8_t {
    Automatic = 0,   // Released at statement end
    Transaction = 1, // Released at transaction end
    Explicit = 2     // Explicitly released
};

// ===== Page Size =====
constexpr size_t PAGE_SIZE = 8 * 1024; // 8KB pages
constexpr size_t DEFAULT_SLOT_SIZE = 128;

// ===== Constants =====
constexpr SCN INVALID_SCN = 0;
constexpr TransactionId INVALID_TX_ID = 0;
constexpr RowId INVALID_ROW_ID = 0;

// ===== Concepts =====
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept ValueContainer = requires(T t) {
    { t.size() } -> std::convertible_to<size_t>;
    { t.data() } -> std::convertible_to<const char*>;
};

} // namespace smdb

#endif // SMDB_UTILS_TYPES_H
