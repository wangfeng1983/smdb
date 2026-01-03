#ifndef SMDB_RECOVERY_RECOVERY_MANAGER_H
#define SMDB_RECOVERY_RECOVERY_MANAGER_H

#include "smdb/utils/types.h"
#include <memory>
#include <vector>
#include <string>

namespace smdb {

// Forward declarations
class IRedoLog;
class IMemManager;

/**
 * @brief Checkpoint information
 */
struct CheckpointInfo {
    SCN scn;
    std::chrono::system_clock::time_point timestamp;
    size_t dirty_pages_flushed;
    bool is_complete;
};

/**
 * @brief Recovery statistics
 */
struct RecoveryStats {
    size_t redo_logs_processed = 0;
    size_t undo_logs_processed = 0;
    size_t pages_recovered = 0;
    size_t transactions_replayed = 0;
    size_t transactions_rolled_back = 0;
    std::chrono::microseconds duration;
};

/**
 * @brief Recovery Manager Interface
 *
 * Manages database recovery using ARIES-style algorithm
 */
class IRecoveryManager {
public:
    virtual ~IRecoveryManager() = default;

    // ===== Initialization =====

    /**
     * @brief Initialize recovery manager
     * @return Result<void> Success or error message
     */
    virtual Result<void> initialize() = 0;

    /**
     * @brief Perform database recovery
     * @return Result<RecoveryStats> Recovery statistics or error
     */
    virtual Result<RecoveryStats> recover() = 0;

    // ===== Checkpoint Management =====

    /**
     * @brief Perform a checkpoint
     * @param type Checkpoint type ("fuzzy" or "consistent")
     * @return Result<CheckpointInfo> Checkpoint info or error
     */
    virtual Result<CheckpointInfo> checkpoint(const std::string& type = "fuzzy") = 0;

    /**
     * @brief Get last checkpoint info
     * @return Checkpoint info
     */
    virtual CheckpointInfo getLastCheckpoint() const = 0;

    /**
     * @brief Schedule automatic checkpoint
     * @param interval_seconds Interval between checkpoints
     */
    virtual void scheduleCheckpoint(size_t interval_seconds) = 0;

    // ===== Log Management =====

    /**
     * @brief Get redo log manager
     * @return Redo log manager
     */
    virtual IRedoLog* getRedoLog() = 0;

    /**
     * @brief Flush logs to disk
     * @param scn Flush up to this SCN
     * @return Result<void> Success or error message
     */
    virtual Result<void> flushLogs(SCN scn) = 0;

    // ===== Analysis Phase =====

    /**
     * @brief Analyze log files
     * @return Result<void> Success or error message
     */
    virtual Result<void> analyzePhase() = 0;

    /**
     * @brief Get oldest active transaction from analysis
     * @return Transaction ID
     */
    virtual TransactionId getOldestActiveTx() const = 0;

    // ===== Redo Phase =====

    /**
     * @brief Redo phase
     * @param start_scn Start SCN
     * @return Result<size_t> Number of logs redone or error
     */
    virtual Result<size_t> redoPhase(SCN start_scn) = 0;

    // ===== Undo Phase =====

    /**
     * @brief Undo phase
     * @return Result<size_t> Number of logs undone or error
     */
    virtual Result<size_t> undoPhase() = 0;

    // ===== Statistics =====

    /**
     * @brief Get recovery statistics
     * @return Recovery statistics
     */
    virtual RecoveryStats getStats() const = 0;
};

/**
 * @brief Redo Log Entry
 */
struct RedoLogEntry {
    SCN scn;
    TransactionId tx_id;
    OperationType type;
    std::string table_name;
    RowId row_id;
    std::vector<uint8_t> before_image;  // For undo
    std::vector<uint8_t> after_image;   // For redo
    size_t size;
};

/**
 * @brief Redo Log Interface
 *
 * Manages write-ahead logging (WAL)
 */
class IRedoLog {
public:
    virtual ~IRedoLog() = default;

    // ===== Log Writing =====

    /**
     * @brief Write a log entry
     * @param entry Log entry
     * @return Result<SCN> SCN of log entry or error
     */
    virtual Result<SCN> write(const RedoLogEntry& entry) = 0;

    /**
     * @brief Write multiple log entries atomically
     * @param entries Vector of log entries
     * @return Result<SCN> Last SCN or error
     */
    virtual Result<SCN> writeBatch(const std::vector<RedoLogEntry>& entries) = 0;

    /**
     * @ Flush log buffer to disk
     * @return Result<void> Success or error message
     */
    virtual Result<void> flush() = 0;

    // ===== Log Reading =====

    /**
     * @brief Read log entry by SCN
     * @param scn SCN
     * @return Result<RedoLogEntry> Log entry or error
     */
    virtual Result<RedoLogEntry> read(SCN scn) = 0;

    /**
     * @brief Read log entries in range
     * @param start_scn Start SCN (inclusive)
     * @param end_scn End SCN (exclusive)
     * @return Result<std::vector<RedoLogEntry>> Log entries or error
     */
    virtual Result<std::vector<RedoLogEntry>> readRange(SCN start_scn, SCN end_scn) = 0;

    /**
     * @brief Get all log entries for a transaction
     * @param tx_id Transaction ID
     * @return Result<std::vector<RedoLogEntry>> Log entries or error
     */
    virtual Result<std::vector<RedoLogEntry>> readTransaction(TransactionId tx_id) = 0;

    // ===== Truncation =====

    /**
     * @brief Truncate logs up to SCN
     * @param scn SCN
     * @return Result<void> Success or error message
     */
    virtual Result<void> truncate(SCN scn) = 0;

    /**
     * @brief Truncate all logs (for testing)
     * @return Result<void> Success or error message
     */
    virtual Result<void> truncateAll() = 0;

    // ===== Metadata =====

    /**
     * @brief Get current SCN
     * @return Current SCN
     */
    virtual SCN getCurrentSCN() const = 0;

    /**
     * @brief Get log file size
     * @return Size in bytes
     */
    virtual size_t getLogSize() const = 0;

    /**
     * @brief Get number of log entries
     * @return Number of entries
     */
    virtual size_t getEntryCount() const = 0;

    /**
     * @brief Check if log buffer needs flush
     * @return true if flush needed
     */
    virtual bool needsFlush() const = 0;
};

/**
 * @brief Undo Log Entry
 */
struct UndoLogEntry {
    SCN scn;
    TransactionId tx_id;
    OperationType type;
    std::string table_name;
    RowId row_id;
    std::vector<uint8_t> before_image;
};

/**
 * @brief Undo Log Interface
 */
class IUndoLog {
public:
    virtual ~IUndoLog() = default;

    /**
     * @brief Write undo log
     * @param entry Undo log entry
     * @return Result<void> Success or error message
     */
    virtual Result<void> write(const UndoLogEntry& entry) = 0;

    /**
     * @brief Get undo logs for transaction
     * @param tx_id Transaction ID
     * @return Vector of undo log entries (in reverse order)
     */
    virtual std::vector<UndoLogEntry> getUndoLogs(TransactionId tx_id) = 0;

    /**
     * @brief Clear undo logs for transaction
     * @param tx_id Transaction ID
     */
    virtual void clear(TransactionId tx_id) = 0;
};

} // namespace smdb

#endif // SMDB_RECOVERY_RECOVERY_MANAGER_H
