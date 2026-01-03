/**
 * SMDB Example Program
 *
 * This example demonstrates the usage of SMDB's modern C++21 API
 * with database operations, transactions, and indexing.
 */

#include "smdb/core/database.h"
#include "smdb/core/table.h"
#include "smdb/transaction/transaction.h"
#include "smdb/index/index.h"
#include <iostream>
#include <print>

using namespace smdb;

int main() {
    std::println("SMDB Example - Modern C++21 In-Memory Database");
    std::println("================================================\n");

    // Step 1: Create database configuration
    DatabaseConfig config;
    config.name = "example_db";
    config.memory_pool_size = 100 * 1024 * 1024; // 100MB
    config.data_dir = "./data";
    config.enable_persistence = true;
    config.enable_recovery = true;
    config.checkpoint_interval_sec = 60;

    std::println("Creating database: {}", config.name);

    // Step 2: Create database instance
    auto db_result = DatabaseFactory::create(config);
    if (!db_result) {
        std::println("Failed to create database: {}", db_result.error());
        return 1;
    }

    auto& db = db_result.value();

    // Step 3: Initialize and start database
    auto init_result = db->initialize();
    if (!init_result) {
        std::println("Failed to initialize database: {}", init_result.error());
        return 1;
    }

    auto start_result = db->start();
    if (!start_result) {
        std::println("Failed to start database: {}", start_result.error());
        return 1;
    }

    std::println("Database started successfully");

    // Step 4: Create a table with schema
    std::println("\n--- Creating table ---");

    TableSchema schema;
    schema.name = "users";
    schema.columns = {
        {"id", ValueType::Int64, 0, false, true},
        {"name", ValueType::String, 100, false, false},
        {"email", ValueType::String, 255, true, false},
        {"age", ValueType::Int32, 0, true, false},
        {"balance", ValueType::Double, 0, true, false}
    };

    auto table_result = db->createTable(schema.name, schema.columns);
    if (!table_result) {
        std::println("Failed to create table: {}", table_result.error());
        return 1;
    }

    auto& users_table = table_result.value();
    std::println("Table '{}' created successfully", schema.name);

    // Step 5: Create indexes
    std::println("\n--- Creating indexes ---");

    // Primary key index (id)
    auto pk_index_result = users_table->createIndex(
        "pk_users_id",
        {"id"},
        IndexType::BTree,
        true  // unique
    );
    if (pk_index_result) {
        std::println("Primary key index created");
    }

    // Email index (unique)
    auto email_index_result = users_table->createIndex(
        "idx_users_email",
        {"email"},
        IndexType::Hash,
        true  // unique
    );
    if (email_index_result) {
        std::println("Email index created");
    }

    // Age index (for range queries)
    auto age_index_result = users_table->createIndex(
        "idx_users_age",
        {"age"},
        IndexType::BTree,
        false  // non-unique
    );
    if (age_index_result) {
        std::println("Age index created");
    }

    // Step 6: Insert data using transaction
    std::println("\n--- Inserting data ---");

    auto tx_result = db->beginTransaction();
    if (!tx_result) {
        std::println("Failed to begin transaction: {}", tx_result.error());
        return 1;
    }

    auto& tx = tx_result.value();

    // Insert user 1
    Row user1;
    user1.id = 1; // Will be assigned by database
    user1.values = {
        {"id", int64_t(1)},
        {"name", std::string("Alice Johnson")},
        {"email", std::string("alice@example.com")},
        {"age", int32_t(30)},
        {"balance", 1000.50}
    };

    auto insert1_result = users_table->insert(user1, tx.get());
    if (insert1_result) {
        std::println("Inserted user: Alice (id: {})", insert1_result.value());
    }

    // Insert user 2
    Row user2;
    user2.values = {
        {"id", int64_t(2)},
        {"name", std::string("Bob Smith")},
        {"email", std::string("bob@example.com")},
        {"age", int32_t(25)},
        {"balance", 2500.75}
    };

    auto insert2_result = users_table->insert(user2, tx.get());
    if (insert2_result) {
        std::println("Inserted user: Bob (id: {})", insert2_result.value());
    }

    // Insert user 3
    Row user3;
    user3.values = {
        {"id", int64_t(3)},
        {"name", std::string("Charlie Brown")},
        {"email", std::string("charlie@example.com")},
        {"age", int32_t(35)},
        {"balance", 500.25}
    };

    auto insert3_result = users_table->insert(user3, tx.get());
    if (insert3_result) {
        std::println("Inserted user: Charlie (id: {})", insert3_result.value());
    }

    // Commit transaction
    auto commit_result = tx->commit();
    if (commit_result) {
        std::println("Transaction committed successfully");
    } else {
        std::println("Failed to commit: {}", commit_result.error());
    }

    // Step 7: Query data
    std::println("\n--- Querying data ---");

    // Get user by ID
    auto get_result = users_table->get(1);
    if (get_result) {
        const auto& user = get_result.value();
        std::println("Found user: {} (age: {}, balance: {})",
            std::get<std::string>(user.values.at("name")),
            std::get<int32_t>(user.values.at("age")),
            std::get<double>(user.values.at("balance"))
        );
    }

    // Step 8: Scan with predicate
    std::println("\n--- Scanning users over 30 ---");

    auto scan_result = users_table->scan([](const Row& row) {
        auto age_it = row.values.find("age");
        if (age_it != row.values.end()) {
            if (std::holds_alternative<int32_t>(age_it->second)) {
                return std::get<int32_t>(age_it->second) > 30;
            }
        }
        return false;
    });

    if (scan_result) {
        for (const auto& row : scan_result.value()) {
            auto name = std::get<std::string>(row.values.at("name"));
            auto age = std::get<int32_t>(row.values.at("age"));
            std::println("  - {}, age {}", name, age);
        }
    }

    // Step 9: Update data in transaction
    std::println("\n--- Updating data ---");

    auto tx2_result = db->beginTransaction();
    if (tx2_result) {
        auto& tx2 = tx2_result.value();

        auto update_result = users_table->update(1,
            {{"balance", 1500.75}},
            tx2.get()
        );

        if (update_result) {
            std::println("Updated Alice's balance to 1500.75");
        }

        tx2->commit();
        std::println("Update transaction committed");
    }

    // Step 10: Display database statistics
    std::println("\n--- Database Statistics ---");

    auto stats = db->getStats();
    std::println("Total tables: {}", stats.total_tables);
    std::println("Total indexes: {}", stats.total_indexes);
    std::println("Total rows: {}", stats.total_rows);
    std::println("Memory used: {} MB", stats.memory_used / (1024 * 1024));
    std::println("Active transactions: {}", stats.active_transactions);
    std::println("Current SCN: {}", stats.current_scn);

    // Step 11: Test rollback
    std::println("\n--- Testing transaction rollback ---");

    auto tx3_result = db->beginTransaction();
    if (tx3_result) {
        auto& tx3 = tx3_result.value();

        // Create savepoint
        tx3->createSavepoint("before_delete");

        // Delete user
        auto delete_result = users_table->remove(2, tx3.get());
        if (delete_result) {
            std::println("Deleted Bob");
        }

        // Rollback to savepoint
        auto rollback_result = tx3->rollbackToSavepoint("before_delete");
        if (rollback_result) {
            std::println("Rolled back to savepoint - Bob is back");
        }

        tx3->commit();
    }

    // Step 12: Perform checkpoint
    std::println("\n--- Performing checkpoint ---");

    auto checkpoint_result = db->checkpoint();
    if (checkpoint_result) {
        std::println("Checkpoint completed successfully");
    }

    // Step 13: Shutdown
    std::println("\n--- Shutting down database ---");

    db->stop();
    db->shutdown();

    std::println("\nDatabase shutdown complete");
    std::println("\n==============================================");
    std::println("SMDB Example completed successfully!");
    std::println("==============================================");

    return 0;
}

/*
 * Expected Output:
 *
 * SMDB Example - Modern C++21 In-Memory Database
 * ================================================
 *
 * Creating database: example_db
 * Database started successfully
 *
 * --- Creating table ---
 * Table 'users' created successfully
 *
 * --- Creating indexes ---
 * Primary key index created
 * Email index created
 * Age index created
 *
 * --- Inserting data ---
 * Inserted user: Alice (id: 1)
 * Inserted user: Bob (id: 2)
 * Inserted user: Charlie (id: 3)
 * Transaction committed successfully
 *
 * --- Querying data ---
 * Found user: Alice Johnson (age: 30, balance: 1000.50)
 *
 * --- Scanning users over 30 ---
 *   - Charlie Brown, age 35
 *
 * --- Updating data ---
 * Updated Alice's balance to 1500.75
 * Update transaction committed
 *
 * --- Testing transaction rollback ---
 * Deleted Bob
 * Rolled back to savepoint - Bob is back
 *
 * --- Performing checkpoint ---
 * Checkpoint completed successfully
 *
 * --- Shutting down database ---
 * Database shutdown complete
 *
 * ==============================================
 * SMDB Example completed successfully!
 * ==============================================
 */
