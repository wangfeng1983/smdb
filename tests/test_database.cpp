#include "memorydb/memorydb.h"
#include <gtest/gtest.h>

using namespace memorydb;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = createDatabase();
    }

    std::unique_ptr<Database> db_;
};

TEST_F(DatabaseTest, PutAndGet) {
    EXPECT_TRUE(db_->put("key1", 42));
    Value value;
    EXPECT_TRUE(db_->get("key1", value));
    EXPECT_EQ(std::get<int64_t>(value), 42);
}

TEST_F(DatabaseTest, GetNonExistentKey) {
    Value value;
    EXPECT_FALSE(db_->get("nonexistent", value));
}

TEST_F(DatabaseTest, UpdateExistingKey) {
    db_->put("key1", 100);
    db_->put("key1", 200);
    Value value;
    db_->get("key1", value);
    EXPECT_EQ(std::get<int64_t>(value), 200);
}

TEST_F(DatabaseTest, RemoveKey) {
    db_->put("key1", 42);
    EXPECT_TRUE(db_->remove("key1"));
    EXPECT_FALSE(db_->exists("key1"));
    EXPECT_FALSE(db_->remove("key1")); // Remove again
}

TEST_F(DatabaseTest, Exists) {
    EXPECT_FALSE(db_->exists("key1"));
    db_->put("key1", 42);
    EXPECT_TRUE(db_->exists("key1"));
}

TEST_F(DatabaseTest, Clear) {
    db_->put("key1", 1);
    db_->put("key2", 2);
    db_->put("key3", 3);
    EXPECT_EQ(db_->size(), 3);

    db_->clear();
    EXPECT_EQ(db_->size(), 0);
    EXPECT_FALSE(db_->exists("key1"));
}

TEST_F(DatabaseTest, DifferentValueTypes) {
    db_->put("int", 42);
    db_->put("double", 3.14);
    db_->put("string", std::string("hello"));
    db_->put("bool", true);

    Value v1, v2, v3, v4;
    EXPECT_TRUE(db_->get("int", v1));
    EXPECT_TRUE(db_->get("double", v2));
    EXPECT_TRUE(db_->get("string", v3));
    EXPECT_TRUE(db_->get("bool", v4));

    EXPECT_EQ(std::get<int64_t>(v1), 42);
    EXPECT_DOUBLE_EQ(std::get<double>(v2), 3.14);
    EXPECT_EQ(std::get<std::string>(v3), "hello");
    EXPECT_EQ(std::get<bool>(v4), true);
}

TEST_F(DatabaseTest, TransactionCommit) {
    db_->put("key1", 100);

    EXPECT_TRUE(db_->beginTransaction());
    db_->put("key1", 200);
    db_->put("key2", 300);
    EXPECT_TRUE(db_->commitTransaction());

    Value v1, v2;
    db_->get("key1", v1);
    db_->get("key2", v2);
    EXPECT_EQ(std::get<int64_t>(v1), 200);
    EXPECT_EQ(std::get<int64_t>(v2), 300);
}

TEST_F(DatabaseTest, TransactionRollback) {
    db_->put("key1", 100);

    EXPECT_TRUE(db_->beginTransaction());
    db_->put("key1", 200);
    db_->put("key2", 300);
    EXPECT_TRUE(db_->rollbackTransaction());

    Value v1;
    EXPECT_TRUE(db_->get("key1", v1));
    EXPECT_EQ(std::get<int64_t>(v1), 100);
    EXPECT_FALSE(db_->exists("key2"));
}

TEST_F(DatabaseTest, NestedTransaction) {
    EXPECT_TRUE(db_->beginTransaction());
    EXPECT_FALSE(db_->beginTransaction()); // Cannot nest
    EXPECT_TRUE(db_->commitTransaction());
}

TEST_F(DatabaseTest, Size) {
    EXPECT_EQ(db_->size(), 0);
    db_->put("key1", 1);
    EXPECT_EQ(db_->size(), 1);
    db_->put("key2", 2);
    EXPECT_EQ(db_->size(), 2);
    db_->put("key3", 3);
    EXPECT_EQ(db_->size(), 3);
    db_->remove("key1");
    EXPECT_EQ(db_->size(), 2);
}
