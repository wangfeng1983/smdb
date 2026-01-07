/**
 * @file test_mem_manager.cpp
 * @brief Unit tests for Memory Manager
 */

#include "smdb/storage/mem_manager.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace smdb {
namespace test {

class MemManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create memory manager
        auto result = MemManagerFactory::create();
        ASSERT_TRUE(result) << "Failed to create memory manager";

        mem_mgr_ = std::move(*result);

        // Initialize with 10MB pool
        auto init_result = mem_mgr_->initialize(10 * 1024 * 1024);
        ASSERT_TRUE(init_result) << "Failed to initialize: " << init_result.error();
    }

    void TearDown() override {
        if (mem_mgr_) {
            mem_mgr_->close();
        }
    }

    std::unique_ptr<IMemManager> mem_mgr_;
};

// ===== Initialization Tests =====

TEST_F(MemManagerTest, InitializeSuccess) {
    auto stats = mem_mgr_->getStats();
    EXPECT_GT(stats.total_size, 0);
    EXPECT_EQ(stats.page_count, 0);
}

TEST_F(MemManagerTest, InitializeWithTooSmallPool) {
    auto mgr = MemManagerFactory::create();
    ASSERT_TRUE(mgr);

    auto result = (*mgr)->initialize(1024);  // Too small
    EXPECT_FALSE(result);
}

TEST_F(MemManagerTest, DoubleInitializeFails) {
    auto result = mem_mgr_->initialize(10 * 1024 * 1024);
    EXPECT_FALSE(result);
}

// ===== Page Allocation Tests =====

TEST_F(MemManagerTest, AllocatePageInTableSpace) {
    auto result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(result);

    PageId page_id = *result;
    EXPECT_GT(page_id, 0);

    auto stats = mem_mgr_->getStats();
    EXPECT_EQ(stats.page_count, 1);
    EXPECT_EQ(stats.allocation_count, 1);
}

TEST_F(MemManagerTest, AllocatePageInDifferentSpaces) {
    auto page1 = mem_mgr_->allocatePage(SpaceType::Table);
    auto page2 = mem_mgr_->allocatePage(SpaceType::Index);
    auto page3 = mem_mgr_->allocatePage(SpaceType::Control);
    auto page4 = mem_mgr_->allocatePage(SpaceType::Undo);
    auto page5 = mem_mgr_->allocatePage(SpaceType::Redo);

    ASSERT_TRUE(page1);
    ASSERT_TRUE(page2);
    ASSERT_TRUE(page3);
    ASSERT_TRUE(page4);
    ASSERT_TRUE(page5);

    auto stats = mem_mgr_->getStats();
    EXPECT_EQ(stats.page_count, 5);
}

TEST_F(MemManagerTest, GetPageSuccess) {
    auto alloc_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(alloc_result);

    PageId page_id = *alloc_result;
    auto get_result = mem_mgr_->getPage(page_id);
    ASSERT_TRUE(get_result);

    void* page_ptr = *get_result;
    EXPECT_NE(page_ptr, nullptr);
}

TEST_F(MemManagerTest, GetPageNotFound) {
    auto result = mem_mgr_->getPage(999999);
    EXPECT_FALSE(result);
}

TEST_F(MemManagerTest, FreePageSuccess) {
    auto alloc_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(alloc_result);

    PageId page_id = *alloc_result;
    auto free_result = mem_mgr_->freePage(page_id);
    EXPECT_TRUE(free_result);

    auto stats = mem_mgr_->getStats();
    EXPECT_EQ(stats.page_count, 0);  // Page is freed
    EXPECT_EQ(stats.deallocation_count, 1);
}

TEST_F(MemManagerTest, FreePageNotFound) {
    auto result = mem_mgr_->freePage(999999);
    EXPECT_FALSE(result);
}

TEST_F(MemManagerTest, MarkAndGetPageInfo) {
    auto alloc_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(alloc_result);

    PageId page_id = *alloc_result;

    // Mark as dirty
    mem_mgr_->markDirty(page_id, 100);

    // Get page info
    auto info_result = mem_mgr_->getPageInfo(page_id);
    ASSERT_TRUE(info_result);

    auto info = *info_result;
    EXPECT_EQ(info.page_id, page_id);
    EXPECT_EQ(info.space, SpaceType::Table);
    EXPECT_GT(info.size, 0);
    EXPECT_TRUE(info.is_dirty);
    EXPECT_EQ(info.last_scn, 100);
}

// ===== Slot Allocation Tests =====

TEST_F(MemManagerTest, AllocateSlotSuccess) {
    // Allocate a page first
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    // Allocate a slot
    auto slot_result = mem_mgr_->allocateSlot(page_id, 256);
    ASSERT_TRUE(slot_result);

    SlotId slot_id = *slot_result;
    EXPECT_GT(slot_id, 0);
}

TEST_F(MemManagerTest, GetSlotSuccess) {
    // Allocate a page first
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    // Allocate a slot
    auto slot_result = mem_mgr_->allocateSlot(page_id, 256);
    ASSERT_TRUE(slot_result);

    // Get slot data
    auto get_result = mem_mgr_->getSlot(page_id, *slot_result);
    ASSERT_TRUE(get_result);

    void* slot_ptr = *get_result;
    EXPECT_NE(slot_ptr, nullptr);

    // Write data to slot
    std::string test_data = "Hello, World!";
    std::memcpy(slot_ptr, test_data.c_str(), test_data.size());

    // Read back
    std::string read_data(static_cast<char*>(slot_ptr), test_data.size());
    EXPECT_EQ(read_data, test_data);
}

TEST_F(MemManagerTest, AllocateMultipleSlots) {
    // Allocate a page first
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    // Allocate multiple slots
    auto slot1 = mem_mgr_->allocateSlot(page_id, 100);
    auto slot2 = mem_mgr_->allocateSlot(page_id, 200);
    auto slot3 = mem_mgr_->allocateSlot(page_id, 300);

    ASSERT_TRUE(slot1);
    ASSERT_TRUE(slot2);
    ASSERT_TRUE(slot3);

    // Verify they have different IDs
    EXPECT_NE(*slot1, *slot2);
    EXPECT_NE(*slot2, *slot3);
}

TEST_F(MemManagerTest, GetSlotNotFound) {
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    auto result = mem_mgr_->getSlot(page_id, 999999);
    EXPECT_FALSE(result);
}

// ===== Statistics Tests =====

TEST_F(MemManagerTest, GetStatsAfterAllocation) {
    auto stats_before = mem_mgr_->getStats();

    // Allocate some pages
    for (int i = 0; i < 10; ++i) {
        auto result = mem_mgr_->allocatePage(SpaceType::Table);
        ASSERT_TRUE(result);
    }

    auto stats_after = mem_mgr_->getStats();
    EXPECT_EQ(stats_after.page_count, stats_before.page_count + 10);
    EXPECT_EQ(stats_after.allocation_count, stats_before.allocation_count + 10);
    EXPECT_GT(stats_after.used_size, stats_before.used_size);
}

TEST_F(MemManagerTest, GetSpaceStats) {
    // Allocate pages in different spaces
    mem_mgr_->allocatePage(SpaceType::Table);
    mem_mgr_->allocatePage(SpaceType::Table);
    mem_mgr_->allocatePage(SpaceType::Index);

    auto table_stats = mem_mgr_->getSpaceStats(SpaceType::Table);
    EXPECT_EQ(table_stats.page_count, 2);

    auto index_stats = mem_mgr_->getSpaceStats(SpaceType::Index);
    EXPECT_EQ(index_stats.page_count, 1);
}

// ===== Dirty Page Tests =====

TEST_F(MemManagerTest, GetDirtyPages) {
    // Allocate pages
    auto page1 = mem_mgr_->allocatePage(SpaceType::Table);
    auto page2 = mem_mgr_->allocatePage(SpaceType::Table);
    auto page3 = mem_mgr_->allocatePage(SpaceType::Table);

    ASSERT_TRUE(page1);
    ASSERT_TRUE(page2);
    ASSERT_TRUE(page3);

    // Mark some as dirty
    mem_mgr_->markDirty(*page1, 100);
    mem_mgr_->markDirty(*page3, 200);

    // Get dirty pages
    auto dirty_pages = mem_mgr_->getDirtyPages();
    EXPECT_EQ(dirty_pages.size(), 2);
}

TEST_F(MemManagerTest, ClearDirtyFlags) {
    // Allocate page and mark as dirty
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);

    PageId page_id = *page_result;
    mem_mgr_->markDirty(page_id, 100);

    // Clear dirty flags
    mem_mgr_->clearDirtyFlags({page_id});

    // Check if still dirty
    auto dirty_pages = mem_mgr_->getDirtyPages();
    EXPECT_EQ(dirty_pages.size(), 0);
}

TEST_F(MemManagerTest, FlushDirtyPages) {
    // Allocate pages
    auto page1 = mem_mgr_->allocatePage(SpaceType::Table);
    auto page2 = mem_mgr_->allocatePage(SpaceType::Table);

    ASSERT_TRUE(page1);
    ASSERT_TRUE(page2);

    // Mark as dirty
    mem_mgr_->markDirty(*page1, 100);
    mem_mgr_->markDirty(*page2, 200);

    // Flush dirty pages
    auto flush_result = mem_mgr_->flushDirtyPages(200);
    ASSERT_TRUE(flush_result);

    size_t flushed_count = *flush_result;
    EXPECT_EQ(flushed_count, 2);
}

// ===== Concurrent Tests =====

TEST_F(MemManagerTest, ConcurrentPageAllocation) {
    const int num_threads = 4;
    const int pages_per_thread = 10;
    std::vector<std::thread> threads;
    std::vector<std::vector<PageId>> page_ids(num_threads);

    // Allocate pages concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, pages_per_thread, &page_ids]() {
            for (int j = 0; j < pages_per_thread; ++j) {
                auto result = mem_mgr_->allocatePage(SpaceType::Table);
                if (result) {
                    page_ids[i].push_back(*result);
                }
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify allocations
    auto stats = mem_mgr_->getStats();
    EXPECT_EQ(stats.page_count, num_threads * pages_per_thread);
    EXPECT_EQ(stats.allocation_count, num_threads * pages_per_thread);

    // Free all pages
    for (const auto& thread_pages : page_ids) {
        for (PageId page_id : thread_pages) {
            mem_mgr_->freePage(page_id);
        }
    }

    auto stats_after = mem_mgr_->getStats();
    EXPECT_EQ(stats_after.page_count, 0);
}

TEST_F(MemManagerTest, ConcurrentSlotAllocation) {
    const int num_threads = 4;
    const int slots_per_thread = 10;
    std::vector<std::thread> threads;

    // Allocate a page first
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    // Allocate slots concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, page_id, slots_per_thread]() {
            for (int j = 0; j < slots_per_thread; ++j) {
                auto result = mem_mgr_->allocateSlot(page_id, 100);
                EXPECT_TRUE(result);
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Get page info to check usage
    auto info_result = mem_mgr_->getPageInfo(page_id);
    ASSERT_TRUE(info_result);

    auto info = *info_result;
    EXPECT_GT(info.used, sizeof(PageHeader));  // Should have used more space
}

// ===== Edge Cases =====

TEST_F(MemManagerTest, AllocateSlotWithLargeSize) {
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    // Try to allocate a large slot (should succeed if page has space)
    auto slot_result = mem_mgr_->allocateSlot(page_id, 4000);
    EXPECT_TRUE(slot_result);
}

TEST_F(MemManagerTest, WriteAndReadSlotData) {
    auto page_result = mem_mgr_->allocatePage(SpaceType::Table);
    ASSERT_TRUE(page_result);
    PageId page_id = *page_result;

    auto slot_result = mem_mgr_->allocateSlot(page_id, 256);
    ASSERT_TRUE(slot_result);

    auto get_result = mem_mgr_->getSlot(page_id, *slot_result);
    ASSERT_TRUE(get_result);

    void* slot_ptr = *get_result;

    // Write test data
    struct TestData {
        int id;
        double value;
        char name[64];
    };

    TestData data{123, 456.789, "TestRecord"};
    std::memcpy(slot_ptr, &data, sizeof(data));

    // Read back
    TestData* read_data = static_cast<TestData*>(slot_ptr);
    EXPECT_EQ(read_data->id, 123);
    EXPECT_DOUBLE_EQ(read_data->value, 456.789);
    EXPECT_STREQ(read_data->name, "TestRecord");
}

} // namespace test
} // namespace smdb

// Main function forgtest
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
