#include "memorydb/memorydb.h"
#include <stdexcept>

namespace memorydb {

InMemoryDatabase::InMemoryDatabase()
    : in_transaction_(false) {
}

bool InMemoryDatabase::put(const std::string& key, const Value& value) {
    if (key.empty()) {
        return false;
    }

    data_[key] = value;

    if (in_transaction_ && transaction_) {
        transaction_->modified_keys.push_back(key);
    }

    return true;
}

bool InMemoryDatabase::get(const std::string& key, Value& value) const {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    value = it->second;
    return true;
}

bool InMemoryDatabase::remove(const std::string& key) {
    auto it = data_.find(key);
    if (it == data_.end()) {
        return false;
    }

    data_.erase(it);

    if (in_transaction_ && transaction_) {
        transaction_->modified_keys.push_back(key);
    }

    return true;
}

bool InMemoryDatabase::exists(const std::string& key) const {
    return data_.find(key) != data_.end();
}

void InMemoryDatabase::clear() {
    data_.clear();

    if (in_transaction_ && transaction_) {
        transaction_->modified_keys.clear();
    }
}

size_t InMemoryDatabase::size() const {
    return data_.size();
}

bool InMemoryDatabase::beginTransaction() {
    if (in_transaction_) {
        return false; // Already in transaction
    }

    transaction_ = std::make_unique<TransactionState>();

    // Create snapshot
    for (const auto& [key, value] : data_) {
        transaction_->snapshot[key] = value;
    }

    in_transaction_ = true;
    return true;
}

bool InMemoryDatabase::commitTransaction() {
    if (!in_transaction_) {
        return false; // No transaction to commit
    }

    transaction_.reset();
    in_transaction_ = false;
    return true;
}

bool InMemoryDatabase::rollbackTransaction() {
    if (!in_transaction_) {
        return false; // No transaction to rollback
    }

    // Restore snapshot
    data_.clear();
    for (const auto& [key, value] : transaction_->snapshot) {
        data_[key] = value;
    }

    transaction_.reset();
    in_transaction_ = false;
    return true;
}

std::unique_ptr<Database> createDatabase() {
    return std::make_unique<InMemoryDatabase>();
}

} // namespace memorydb
