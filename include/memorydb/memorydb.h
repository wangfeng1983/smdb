#ifndef MEMORYDB_MEMORYDB_H
#define MEMORYDB_MEMORYDB_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace memorydb {

// Value type supporting multiple data types
using Value = std::variant<
    std::monostate,
    int64_t,
    double,
    std::string,
    bool
>;

// Database interface
class Database {
public:
    virtual ~Database() = default;

    // Key-Value operations
    virtual bool put(const std::string& key, const Value& value) = 0;
    virtual bool get(const std::string& key, Value& value) const = 0;
    virtual bool remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;

    // Transaction support
    virtual bool beginTransaction() = 0;
    virtual bool commitTransaction() = 0;
    virtual bool rollbackTransaction() = 0;
};

// In-memory database implementation
class InMemoryDatabase : public Database {
public:
    InMemoryDatabase();
    ~InMemoryDatabase() override = default;

    // Key-Value operations
    bool put(const std::string& key, const Value& value) override;
    bool get(const std::string& key, Value& value) const override;
    bool remove(const std::string& key) override;
    bool exists(const std::string& key) const override;
    void clear() override;
    size_t size() const override;

    // Transaction support
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;

private:
    struct TransactionState {
        std::unordered_map<std::string, Value> snapshot;
        std::vector<std::string> modified_keys;
    };

    std::unordered_map<std::string, Value> data_;
    std::unique_ptr<TransactionState> transaction_;
    bool in_transaction_;
};

// Factory function
std::unique_ptr<Database> createDatabase();

} // namespace memorydb

#endif // MEMORYDB_MEMORYDB_H
