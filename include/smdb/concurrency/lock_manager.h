#ifndef SMDB_CONCURRENCY_LOCK_MANAGER_H
#define SMDB_CONCURRENCY_LOCK_MANAGER_H

#include "smdb/utils/types.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace smdb {

// Forward declarations
class ITransaction;

/**
 * @brief Lock request
 */
struct LockRequest {
    TransactionId tx_id;
    LockType type;
    LockDuration duration;
    std::chrono::time_point<std::chrono::system_clock> timestamp;

    LockRequest(TransactionId id, LockType t, LockDuration d)
        : tx_id(id), type(t), duration(d),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Lock entry
 */
struct LockEntry {
    std::string resource_id;  // Table:row_id format
    LockType type;
    std::vector<LockRequest> granted;     // Granted locks
    std::vector<LockRequest> waiting;     // Waiting locks

    LockEntry() : type(LockType::None) {}
};

/**
 * @brief Deadlock information
 */
struct DeadlockInfo {
    std::vector<TransactionId> cycle;
    TransactionId victim;
};

/**
 * @brief Lock Manager Interface
 *
 * Manages locks on database resources with deadlock detection
 */
class ILockManager {
public:
    virtual ~ILockManager() = default;

    // ===== Lock Operations =====

    /**
     * @brief Acquire a lock
     * @param tx_id Transaction ID
     * @param resource_id Resource identifier (table:row format)
     * @param type Lock type
     * @param duration Lock duration
     * @param timeout Timeout in milliseconds
     * @return Result<void> Success or error message
     */
    virtual Result<void> acquireLock(TransactionId tx_id,
                                      const std::string& resource_id,
                                      LockType type,
                                      LockDuration duration,
                                      std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Release a lock
     * @param tx_id Transaction ID
     * @param resource_id Resource identifier
     * @return Result<void> Success or error message
     */
    virtual Result<void> releaseLock(TransactionId tx_id,
                                      const std::string& resource_id) = 0;

    /**
     * @brief Release all locks held by a transaction
     * @param tx_id Transaction ID
     */
    virtual void releaseAllLocks(TransactionId tx_id) = 0;

    // ===== Query Operations =====

    /**
     * @brief Check if a lock is held
     * @param tx_id Transaction ID
     * @param resource_id Resource identifier
     * @return true if lock is held
     */
    virtual bool isLockHeld(TransactionId tx_id,
                           const std::string& resource_id) const = 0;

    /**
     * @brief Check if a lock is compatible
     * @param existing Existing lock type
     * @param requested Requested lock type
     * @return true if compatible
     */
    virtual bool isCompatible(LockType existing, LockType requested) const = 0;

    /**
     * @brief Get all locks held by a transaction
     * @param tx_id Transaction ID
     * @return Vector of resource IDs
     */
    virtual std::vector<std::string> getHeldLocks(TransactionId tx_id) const = 0;

    // ===== Deadlock Detection =====

    /**
     * @brief Detect deadlocks
     * @return Deadlock information if deadlock found
     */
    virtual std::optional<DeadlockInfo> detectDeadlock() = 0;

    /**
     * @brief Set deadlock timeout
     * @param timeout Timeout in milliseconds
     */
    virtual void setDeadlockTimeout(std::chrono::milliseconds timeout) = 0;

    // ===== Statistics =====

    /**
     * @brief Get number of active locks
     * @return Lock count
     */
    virtual size_t getLockCount() const = 0;

    /**
     * @brief Get number of waiting requests
     * @return Waiting count
     */
    virtual size_t getWaitingCount() const = 0;

    /**
     * @brief Dump lock table for debugging
     * @return String representation
     */
    virtual std::string dumpLockTable() const = 0;
};

/**
 * @brief Latch Manager (for internal synchronization)
 *
 * Lightweight latches for in-memory data structures
 */
class ILatchManager {
public:
    virtual ~ILatchManager() = default;

    /**
     * @brief Acquire shared latch
     * @param addr Address to latch
     */
    virtual void acquireShared(void* addr) = 0;

    /**
     * @brief Acquire exclusive latch
     * @param addr Address to latch
     */
    virtual void acquireExclusive(void* addr) = 0;

    /**
     * @brief Release shared latch
     * @param addr Address to latch
     */
    virtual void releaseShared(void* addr) = 0;

    /**
     * @brief Release exclusive latch
     * @param addr Address to latch
     */
    virtual void releaseExclusive(void* addr) = 0;

    /**
     * @brief Try acquire shared latch
     * @param addr Address to latch
     * @return true if acquired
     */
    virtual bool tryAcquireShared(void* addr) = 0;

    /**
     * @brief Try acquire exclusive latch
     * @param addr Address to latch
     * @return true if acquired
     */
    virtual bool tryAcquireExclusive(void* addr) = 0;
};

} // namespace smdb

#endif // SMDB_CONCURRENCY_LOCK_MANAGER_H
