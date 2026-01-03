#ifndef SMDB_TRANSACTION_TRANSACTION_H
#define SMDB_TRANSACTION_TRANSACTION_H

#include "smdb/utils/types.h"
#include "smdb/core/table.h"
#include <memory>
#include <vector>
#include <chrono>
#include <functional>

namespace smdb {

// Forward declarations
class LockManager;

/**
 * @brief Transaction savepoint
 */
struct Savepoint {
    std::string name;
    SCN scn;
    size_t operation_count;
};

/**
 * @brief Transaction log entry
 */
struct LogEntry {
    OperationType type;
    SCN scn;
    std::string table_name;
    RowId row_id;
    Row before_image;  // For undo
    Row after_image;   // For redo
};

/**
 * @brief Transaction Interface
 *
 * Manages transaction lifecycle and provides ACID guarantees
 */
class ITransaction {
public:
    virtual ~ITransaction() = default;

    // ===== Transaction Control =====

    /**
     * @brief Commit the transaction
     * @return Result<void> Success or error message
     */
    virtual Result<void> commit() = 0;

    /**
     * @brief Rollback the transaction
     * @return Result<void> Success or error message
     */
    virtual Result<void> rollback() = 0;

    /**
     * @brief Create a savepoint
     * @param name Savepoint name
     * @return Result<void> Success or error message
     */
    virtual Result<void> createSavepoint(const std::string& name) = 0;

    /**
     * @brief Rollback to a savepoint
     * @param name Savepoint name
     * @return Result<void> Success or error message
     */
    virtual Result<void> rollbackToSavepoint(const std::string& name) = 0;

    /**
     * @brief Release a savepoint
     * @param name Savepoint name
     * @return Result<void> Success or error message
     */
    virtual Result<void> releaseSavepoint(const std::string& name) = 0;

    // ===== Transaction State =====

    /**
     * @brief Get transaction ID
     * @return Transaction ID
     */
    virtual TransactionId getId() const = 0;

    /**
     * @brief Get transaction state
     * @return Current state
     */
    virtual TransactionState getState() const = 0;

    /**
     * @brief Check if transaction is active
     * @return true if active
     */
    virtual bool isActive() const = 0;

    /**
     * @brief Get start SCN
     * @return Start SCN
     */
    virtual SCN getStartSCN() const = 0;

    // ===== Operations =====

    /**
     * @brief Log an operation
     * @param entry Log entry
     * @return Result<void> Success or error message
     */
    virtual Result<void> logOperation(const LogEntry& entry) = 0;

    /**
     * @brief Get all log entries
     * @return Vector of log entries
     */
    virtual const std::vector<LogEntry>& getLogEntries() const = 0;

    // ===== Statistics =====

    /**
     * @brief Get transaction duration
     * @return Duration since start
     */
    virtual std::chrono::microseconds getDuration() const = 0;

    /**
     * @brief Get number of operations
     * @return Operation count
     */
    virtual size_t getOperationCount() const = 0;

    /**
     * @brief Get lock manager
     * @return Lock manager instance
     */
    virtual LockManager* getLockManager() = 0;
};

/**
 * @brief Transaction Manager Interface
 *
 * Manages all transactions in the system
 */
class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    /**
     * @brief Begin a new transaction
     * @return Result<std::shared_ptr<ITransaction>> New transaction or error
     */
    virtual Result<std::shared_ptr<ITransaction>> begin() = 0;

    /**
     * @brief Get a transaction by ID
     * @param id Transaction ID
     * @return Transaction or nullptr
     */
    virtual std::shared_ptr<ITransaction> getTransaction(TransactionId id) = 0;

    /**
     * @brief Get all active transactions
     * @return Vector of active transactions
     */
    virtual std::vector<std::shared_ptr<ITransaction>> getActiveTransactions() = 0;

    /**
     * @brief Get transaction count
     * @return Number of active transactions
     */
    virtual size_t getTransactionCount() const = 0;

    /**
     * @brief Check for deadlocks
     * @return Vector of transaction IDs to rollback
     */
    virtual std::vector<TransactionId> detectDeadlocks() = 0;

    /**
     * @brief Cleanup completed transactions
     */
    virtual void cleanup() = 0;
};

} // namespace smdb

#endif // SMDB_TRANSACTION_TRANSACTION_H
