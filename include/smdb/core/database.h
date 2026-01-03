#ifndef SMDB_CORE_DATABASE_H
#define SMDB_CORE_DATABASE_H

#include "smdb/utils/types.h"
#include <memory>
#include <vector>
#include <functional>

namespace smdb {

// Forward declarations
class Table;
class Transaction;
class MemManager;
class RecoveryManager;

/**
 * @brief Database configuration
 */
struct DatabaseConfig {
    DbName name;
    size_t memory_pool_size = 1024 * 1024 * 1024; // 1GB default
    std::string data_dir = "./data";
    bool enable_persistence = true;
    bool enable_recovery = true;
    size_t checkpoint_interval_sec = 60; // Checkpoint every 60 seconds
    size_t max_connections = 100;
};

/**
 * @brief Database statistics
 */
struct DatabaseStats {
    size_t total_tables = 0;
    size_t total_indexes = 0;
    size_t total_rows = 0;
    size_t memory_used = 0;
    size_t memory_free = 0;
    size_t active_transactions = 0;
    SCN current_scn = INVALID_SCN;
};

/**
 * @brief Core Database Interface
 *
 * This is the main entry point for the SMDB database system.
 * It provides lifecycle management, table management, and statistics.
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;

    // ===== Lifecycle Management =====

    /**
     * @brief Initialize the database
     * @return Result<void> Success or error message
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief Start the database (begin accepting connections)
     * @return Result<void> Success or error message
     */
    virtual Result<void> start() = 0;

    /**
     * @brief Stop the database gracefully
     * @return Result<void> Success or error message
     */
    virtual Result<void> stop() = 0;

    /**
     * @brief Shutdown and cleanup resources
     * @return Result<void> Success or error message
     */
    virtual Result<void> shutdown() = 0;

    // ===== Table Management =====

    /**
     * @brief Create a new table
     * @param name Table name
     * @param schema Column definitions
     * @return Result<std::shared_ptr<Table>> Created table or error
     */
    virtual Result<std::shared_ptr<Table>> createTable(
        const TableName& name,
        const std::vector<ColumnDef>& schema
    ) = 0;

    /**
     * @brief Drop a table
     * @param name Table name
     * @return Result<void> Success or error message
     */
    virtual Result<void> dropTable(const TableName& name) = 0;

    /**
     * @brief Get a table by name
     * @param name Table name
     * @return Result<std::shared_ptr<Table>> Table or error
     */
    virtual Result<std::shared_ptr<Table>> getTable(const TableName& name) = 0;

    /**
     * @brief Check if a table exists
     * @param name Table name
     * @return true if table exists
     */
    virtual bool tableExists(const TableName& name) const = 0;

    /**
     * @brief Get all table names
     * @return Vector of table names
     */
    virtual std::vector<TableName> getTableNames() const = 0;

    // ===== Transaction Management =====

    /**
     * @brief Begin a new transaction
     * @return Result<std::shared_ptr<Transaction>> New transaction or error
     */
    virtual Result<std::shared_ptr<Transaction>> beginTransaction() = 0;

    /**
     * @brief Get current SCN (System Change Number)
     * @return Current SCN
     */
    virtual SCN getCurrentSCN() const = 0;

    // ===== Statistics =====

    /**
     * @brief Get database statistics
     * @return Database statistics
     */
    virtual DatabaseStats getStats() const = 0;

    // ===== Checkpoint & Recovery =====

    /**
     * @brief Trigger a checkpoint
     * @return Result<void> Success or error message
     */
    virtual Result<void> checkpoint() = 0;

    /**
     * @brief Get the recovery manager
     * @return Recovery manager instance
     */
    virtual RecoveryManager* getRecoveryManager() = 0;

    /**
     * @brief Get the memory manager
     * @return Memory manager instance
     */
    virtual MemManager* getMemManager() = 0;
};

/**
 * @brief Database Factory
 */
class DatabaseFactory {
public:
    /**
     * @brief Create a new database instance
     * @param config Database configuration
     * @return Result<std::unique_ptr<IDatabase>> Database instance or error
     */
    static Result<std::unique_ptr<IDatabase>> create(const DatabaseConfig& config);
};

} // namespace smdb

#endif // SMDB_CORE_DATABASE_H
