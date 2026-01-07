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
// #include <expected>  // C++23 only, use custom Result instead

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

// Forward declaration for Result<void>
template<typename T>
class Result;

// ===== Result Type (C++20 compatible alternative to std::expected) =====

// Specialization for void return type
template<>
class Result<void> {
private:
    std::variant<std::monostate, std::string> value_;

public:
    // Constructors for success case
    Result(std::monostate) : value_(std::monostate{}) {}
    Result() : value_(std::monostate{}) {}

    // Constructor for error case
    Result(std::string error) : value_(std::move(error)) {}
    Result(const char* error) : value_(std::string(error)) {}

    // Check if result holds a value
    bool has_value() const noexcept {
        return std::holds_alternative<std::monostate>(value_);
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    // Get error message
    const std::string& error() const & {
        return std::get<std::string>(value_);
    }

    std::string&& error() && {
        return std::get<std::string>(std::move(value_));
    }
};

// General Result template
template<typename T>
class Result {
private:
    std::variant<T, std::string> value_;

public:
    // Constructors for success case (enable_if to disallow string)
    template<typename U = T,
             typename std::enable_if<!std::is_same<U, std::string>::value &&
                                     !std::is_same<U, const char*>::value, int>::type = 0>
    Result(U&& value) : value_(std::forward<U>(value)) {}

    // Constructor for error case (explicit for string types)
    explicit Result(std::string error) : value_(std::move(error)) {}
    explicit Result(const char* error) : value_(std::string(error)) {}

    // Copy and move
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    // Check if result holds a value
    bool has_value() const noexcept {
        return std::holds_alternative<T>(value_);
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    // Get the value (undefined if error)
    const T& operator*() const & {
        return std::get<T>(value_);
    }

    T& operator*() & {
        return std::get<T>(value_);
    }

    T&& operator*() && {
        return std::get<T>(std::move(value_));
    }

    const T&& operator*() const&& {
        return std::get<T>(std::move(value_));
    }

    // Get value or throw if error
    T& value() & {
        if (!has_value()) {
            throw std::runtime_error(std::get<std::string>(value_));
        }
        return std::get<T>(value_);
    }

    const T& value() const & {
        if (!has_value()) {
            throw std::runtime_error(std::get<std::string>(value_));
        }
        return std::get<T>(value_);
    }

    T&& value() && {
        if (!has_value()) {
            throw std::runtime_error(std::get<std::string>(value_));
        }
        return std::get<T>(std::move(value_));
    }

    // Get error message
    const std::string& error() const & {
        return std::get<std::string>(value_);
    }

    std::string&& error() && {
        return std::get<std::string>(std::move(value_));
    }
};

// Helper to create unexpected results (similar to std::unexpected)
struct Unexpected {
    std::string message;
    explicit Unexpected(std::string msg) : message(std::move(msg)) {}
    explicit Unexpected(const char* msg) : message(msg) {}
};

// Macro to simplify error returns
#define SMDB_ERROR(type, msg) Result<type>(std::string(msg))

// Convenience helpers
template<typename T>
Result<T> success(T value) {
    return Result<T>(std::move(value));
}

inline Result<void> success() {
    return Result<void>(std::monostate{});
}

template<typename T>
Result<T> failure(std::string message) {
    return Result<T>(std::move(message));
}

inline Result<void> failure(std::string message) {
    return Result<void>(std::move(message));
}

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
