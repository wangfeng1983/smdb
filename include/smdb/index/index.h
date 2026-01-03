#ifndef SMDB_INDEX_INDEX_H
#define SMDB_INDEX_INDEX_H

#include "smdb/utils/types.h"
#include "smdb/core/table.h"
#include <memory>
#include <vector>
#include <variant>

namespace smdb {

/**
 * @brief Index key
 */
using IndexKey = std::variant<
    int64_t,
    double,
    std::string,
    std::vector<Value>
>;

/**
 * @brief Index entry
 */
struct IndexEntry {
    IndexKey key;
    RowId row_id;
};

/**
 * @brief Index definition
 */
struct IndexDef {
    IndexName name;
    TableName table_name;
    std::vector<ColumnName> columns;
    IndexType type;
    bool unique;
    bool primary;  // Primary key index

    bool isComposite() const {
        return columns.size() > 1;
    }

    size_t getColumnCount() const {
        return columns.size();
    }
};

/**
 * @brief Index statistics
 */
struct IndexStats {
    size_t entry_count = 0;
    size_t tree_height = 0;     // For B-tree
    size_t bucket_count = 0;    // For hash
    size_t memory_used = 0;
    size_t disk_used = 0;
};

/**
 * @brief Index Interface
 *
 * Provides index operations for fast data access
 */
class IIndex {
public:
    virtual ~IIndex() = default;

    // ===== Basic Operations =====

    /**
     * @brief Insert an entry
     * @param key Index key
     * @param row_id Row ID
     * @return Result<void> Success or error message
     */
    virtual Result<void> insert(const IndexKey& key, RowId row_id) = 0;

    /**
     * @brief Delete an entry
     * @param key Index key
     * @param row_id Row ID
     * @return Result<void> Success or error message
     */
    virtual Result<void> remove(const IndexKey& key, RowId row_id) = 0;

    /**
     * @brief Update an entry
     * @param old_key Old key
     * @param new_key New key
     * @param row_id Row ID
     * @return Result<void> Success or error message
     */
    virtual Result<void> update(const IndexKey& old_key,
                                const IndexKey& new_key,
                                RowId row_id) = 0;

    // ===== Query Operations =====

    /**
     * @brief Exact match lookup
     * @param key Index key
     * @return Result<std::vector<RowId>> Matching row IDs or error
     */
    virtual Result<std::vector<RowId>> lookup(const IndexKey& key) = 0;

    /**
     * @brief Range query
     * @param start_key Start key (inclusive)
     * @param end_key End key (exclusive)
     * @return Result<std::vector<RowId>> Row IDs in range or error
     */
    virtual Result<std::vector<RowId>> range(const IndexKey& start_key,
                                              const IndexKey& end_key) = 0;

    /**
     * @brief Prefix search (for string indexes)
     * @param prefix Key prefix
     * @return Result<std::vector<RowId>> Matching row IDs or error
     */
    virtual Result<std::vector<RowId>> prefix(const std::string& prefix) = 0;

    /**
     * @brief Get all entries
     * @return Result<std::vector<RowId>> All row IDs or error
     */
    virtual Result<std::vector<RowId>> scanAll() = 0;

    // ===== Metadata =====

    /**
     * @brief Get index definition
     * @return Index definition
     */
    virtual const IndexDef& getDef() const = 0;

    /**
     * @brief Get index name
     * @return Index name
     */
    virtual const IndexName& getName() const = 0;

    /**
     * @brief Get index type
     * @return Index type
     */
    virtual IndexType getType() const = 0;

    /**
     * @brief Check if index is unique
     * @return true if unique
     */
    virtual bool isUnique() const = 0;

    /**
     * @brief Check if index is primary key
     * @return true if primary key
     */
    virtual bool isPrimary() const = 0;

    /**
     * @brief Get index statistics
     * @return Index statistics
     */
    virtual IndexStats getStats() const = 0;

    /**
     * @brief Get entry count
     * @return Number of entries
     */
    virtual size_t size() const = 0;

    /**
     * @brief Clear all entries
     * @return Result<void> Success or error message
     */
    virtual Result<void> clear() = 0;

    // ===== Maintenance =====

    /**
     * @brief Rebuild index
     * @return Result<void> Success or error message
     */
    virtual Result<void> rebuild() = 0;

    /**
     * @brief Compact index (optimize storage)
     * @return Result<void> Success or error message
     */
    virtual Result<void> compact() = 0;
};

/**
 * @brief Index Factory
 */
class IndexFactory {
public:
    /**
     * @brief Create an index
     * @param def Index definition
     * @return Result<std::shared_ptr<IIndex>> Index instance or error
     */
    static Result<std::shared_ptr<IIndex>> create(const IndexDef& def);
};

} // namespace smdb

#endif // SMDB_INDEX_INDEX_H
