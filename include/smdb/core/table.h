#ifndef SMDB_CORE_TABLE_H
#define SMDB_CORE_TABLE_H

#include "smdb/utils/types.h"
#include "smdb/core/database.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>

namespace smdb {

// Forward declarations
class Index;
class Transaction;

/**
 * @brief Row representation
 */
struct Row {
    RowId id;
    std::unordered_map<ColumnName, Value> values;
    SCN create_scn = INVALID_SCN;
    SCN delete_scn = INVALID_SCN; // INVALID_SCN means not deleted
};

/**
 * @brief Table schema
 */
struct TableSchema {
    TableName name;
    std::vector<ColumnDef> columns;

    std::optional<const ColumnDef*> getColumn(const ColumnName& name) const {
        auto it = std::find_if(columns.begin(), columns.end(),
            [&name](const ColumnDef& col) { return col.name == name; });
        if (it != columns.end()) {
            return &(*it);
        }
        return std::nullopt;
    }

    bool hasColumn(const ColumnName& name) const {
        return getColumn(name).has_value();
    }
};

/**
 * @brief Table Interface
 *
 * Provides CRUD operations and index management
 */
class ITable {
public:
    virtual ~ITable() = default;

    // ===== Basic Operations =====

    /**
     * @brief Insert a new row
     * @param row Row to insert
     * @param tx Transaction context
     * @return Result<RowId> Inserted row ID or error
     */
    virtual Result<RowId> insert(const Row& row, Transaction* tx = nullptr) = 0;

    /**
     * @brief Update a row
     * @param id Row ID
     * @param values New values
     * @param tx Transaction context
     * @return Result<void> Success or error message
     */
    virtual Result<void> update(RowId id,
                                const std::unordered_map<ColumnName, Value>& values,
                                Transaction* tx = nullptr) = 0;

    /**
     * @brief Delete a row
     * @param id Row ID
     * @param tx Transaction context
     * @return Result<void> Success or error message
     */
    virtual Result<void> remove(RowId id, Transaction* tx = nullptr) = 0;

    /**
     * @brief Get a row by ID
     * @param id Row ID
     * @param tx Transaction context
     * @return Result<Row> Row or error
     */
    virtual Result<Row> get(RowId id, Transaction* tx = nullptr) = 0;

    // ===== Query Operations =====

    /**
     * @brief Scan all rows
     * @param tx Transaction context
     * @return Result<std::vector<Row>> All rows or error
     */
    virtual Result<std::vector<Row>> scan(Transaction* tx = nullptr) = 0;

    /**
     * @brief Scan rows with predicate
     * @param predicate Filter function
     * @param tx Transaction context
     * @return Result<std::vector<Row>> Filtered rows or error
     */
    virtual Result<std::vector<Row>> scan(
        std::function<bool(const Row&)> predicate,
        Transaction* tx = nullptr
    ) = 0;

    // ===== Index Management =====

    /**
     * @brief Create an index
     * @param name Index name
     * @param columns Columns to index
     * @param type Index type
     * @param unique Whether index is unique
     * @return Result<std::shared_ptr<Index>> Created index or error
     */
    virtual Result<std::shared_ptr<Index>> createIndex(
        const IndexName& name,
        const std::vector<ColumnName>& columns,
        IndexType type = IndexType::BTree,
        bool unique = false
    ) = 0;

    /**
     * @brief Drop an index
     * @param name Index name
     * @return Result<void> Success or error message
     */
    virtual Result<void> dropIndex(const IndexName& name) = 0;

    /**
     * @brief Get an index by name
     * @param name Index name
     * @return Result<std::shared_ptr<Index>> Index or error
     */
    virtual Result<std::shared_ptr<Index>> getIndex(const IndexName& name) = 0;

    /**
     * @brief Get all indexes
     * @return Vector of indexes
     */
    virtual std::vector<std::shared_ptr<Index>> getIndexes() const = 0;

    // ===== Metadata =====

    /**
     * @brief Get table schema
     * @return Table schema
     */
    virtual const TableSchema& getSchema() const = 0;

    /**
     * @brief Get table name
     * @return Table name
     */
    virtual const TableName& getName() const = 0;

    /**
     * @brief Get row count
     * @return Number of rows
     */
    virtual size_t getRowCount() const = 0;

    /**
     * @brief Truncate table (delete all rows)
     * @return Result<void> Success or error message
     */
    virtual Result<void> truncate() = 0;
};

} // namespace smdb

#endif // SMDB_CORE_TABLE_H
