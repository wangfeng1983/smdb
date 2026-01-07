/**
 * @file mem_manager.cpp
 * @brief Memory Manager Implementation
 *
 * SMDB Memory Manager - Manages in-memory storage with page-based allocation
 * Based on MDB's MemManager design but modernized with C++20
 */

#include "smdb/storage/mem_manager.h"
#include "smdb/utils/logger.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace smdb {

// Constants (PAGE_SIZE is defined in types.h)
constexpr size_t MIN_POOL_SIZE = 1024 * 1024;  // 1MB minimum
constexpr size_t MAX_POOL_SIZE = 1024ULL * 1024 * 1024 * 1024; // 1TB maximum

// Page header (stored at the beginning of each page)
struct PageHeader {
    PageId page_id;
    SpaceType space;
    size_t size;
    size_t used;
    bool is_dirty;
    SCN last_scn;
    SlotId next_slot_id;
    uint32_t slot_count;
    uint32_t free_offset;  // Next free space offset

    PageHeader(PageId id, SpaceType s)
        : page_id(id)
        , space(s)
        , size(PAGE_SIZE)
        , used(sizeof(PageHeader))
        , is_dirty(false)
        , last_scn(0)
        , next_slot_id(0)
        , slot_count(0)
        , free_offset(sizeof(PageHeader)) {}
};

// Slot header (stored before each slot data)
struct SlotHeader {
    SlotId slot_id;
    size_t size;
    bool is_free;
    bool is_deleted;
    SCN create_scn;
    SCN delete_scn;
    size_t data_offset;  // Offset from page start

    SlotHeader(SlotId id, size_t sz)
        : slot_id(id)
        , size(sz)
        , is_free(false)
        , is_deleted(false)
        , create_scn(0)
        , delete_scn(0)
        , data_offset(0) {}
};

// Per-space memory pool
class SpacePool {
public:
    SpacePool(SpaceType type, size_t pool_size)
        : space_type_(type)
        , pool_size_(pool_size)
        , used_size_(0)
        , allocation_count_(0)
        , deallocation_count_(0) {
        // Allocate memory for this space
        memory_ = std::make_unique<std::byte[]>(pool_size);
        SMDB_LOG_INFO("Allocated space pool: type=" + std::to_string(static_cast<int>(type)) +
                     ", size=" + std::to_string(pool_size));
    }

    ~SpacePool() {
        SMDB_LOG_INFO("Freeing space pool: type=" + std::to_string(static_cast<int>(space_type_)));
    }

    // Allocate a page
    Result<PageId> allocatePage() {
        std::unique_lock lock(mutex_);

        // Check if we have enough free space
        if (used_size_ + PAGE_SIZE > pool_size_) {
            return Result<PageId>("Out of memory in space: " +
                                   std::to_string(static_cast<int>(space_type_)));
        }

        // Find a free page in the free list
        if (!free_pages_.empty()) {
            PageId page_id = free_pages_.back();
            free_pages_.pop_back();

            // Reinitialize the page
            auto* header = reinterpret_cast<PageHeader*>(memory_.get() + page_id * PAGE_SIZE);
            new (header) PageHeader(page_id, space_type_);

            used_size_ += PAGE_SIZE;
            allocation_count_++;

            SMDB_LOG_DEBUG("Reallocated page: id=" + std::to_string(page_id) +
                          ", space=" + std::to_string(static_cast<int>(space_type_)));
            return page_id;
        }

        // Allocate a new page
        PageId page_id = static_cast<PageId>(pages_.size());

        if (used_size_ + PAGE_SIZE > pool_size_) {
            return Result<PageId>("Out of memory in space: " +
                                   std::to_string(static_cast<int>(space_type_)));
        }

        // Initialize page header
        size_t offset = page_id * PAGE_SIZE;
        auto* header = new (memory_.get() + offset) PageHeader(page_id, space_type_);

        pages_[page_id] = header;
        used_size_ += PAGE_SIZE;
        allocation_count_++;

        SMDB_LOG_DEBUG("Allocated new page: id=" + std::to_string(page_id) +
                      ", space=" + std::to_string(static_cast<int>(space_type_)) +
                      ", offset=" + std::to_string(offset));

        return page_id;
    }

    // Free a page
    Result<void> freePage(PageId page_id) {
        std::unique_lock lock(mutex_);

        auto it = pages_.find(page_id);
        if (it == pages_.end()) {
            return Result<void>("Page not found: " + std::to_string(page_id));
        }

        // Mark page as free
        free_pages_.push_back(page_id);
        used_size_ -= PAGE_SIZE;
        deallocation_count_++;

        SMDB_LOG_DEBUG("Freed page: id=" + std::to_string(page_id) +
                      ", space=" + std::to_string(static_cast<int>(space_type_)));

        return {};
    }

    // Get page pointer
    Result<void*> getPage(PageId page_id) {
        std::shared_lock lock(mutex_);

        auto it = pages_.find(page_id);
        if (it == pages_.end()) {
            return Result<void*>("Page not found: " + std::to_string(page_id));
        }

        return memory_.get() + page_id * PAGE_SIZE;
    }

    // Get page header
    Result<PageHeader*> getPageHeader(PageId page_id) {
        std::shared_lock lock(mutex_);

        auto it = pages_.find(page_id);
        if (it == pages_.end()) {
            return Result<PageHeader*>("Page not found: " + std::to_string(page_id));
        }

        return it->second;
    }

    // Allocate slot in page
    Result<SlotId> allocateSlot(PageId page_id, size_t size) {
        std::unique_lock lock(mutex_);

        auto header_result = getPageHeader(page_id);
        if (!header_result) {
            return Result<SlotId>(header_result.error());
        }

        auto* header = *header_result;

        // Check if page has enough space
        size_t required_size = sizeof(SlotHeader) + size;
        if (header->free_offset + required_size > PAGE_SIZE) {
            return Result<SlotId>("Not enough space in page: " + std::to_string(page_id));
        }

        // Allocate slot
        SlotId slot_id = header->next_slot_id++;

        // Calculate slot data offset
        size_t data_offset = header->free_offset + sizeof(SlotHeader);

        // Write slot header
        auto* slot_header = new (memory_.get() + page_id * PAGE_SIZE + header->free_offset)
                                SlotHeader(slot_id, size);
        slot_header->data_offset = data_offset;

        // Update page header
        header->used += required_size;
        header->free_offset += required_size;
        header->slot_count++;

        SMDB_LOG_DEBUG("Allocated slot: page=" + std::to_string(page_id) +
                      ", slot=" + std::to_string(slot_id) +
                      ", size=" + std::to_string(size) +
                      ", offset=" + std::to_string(data_offset));

        return slot_id;
    }

    // Get slot data
    Result<void*> getSlot(PageId page_id, SlotId slot_id) {
        std::shared_lock lock(mutex_);

        auto header_result = getPageHeader(page_id);
        if (!header_result) {
            return Result<void*>(header_result.error());
        }

        auto* header = *header_result;

        // Scan through slots to find the requested one
        size_t offset = sizeof(PageHeader);
        for (uint32_t i = 0; i < header->slot_count; ++i) {
            auto* slot_header = reinterpret_cast<SlotHeader*>(memory_.get() +
                                                              page_id * PAGE_SIZE + offset);

            if (slot_header->slot_id == slot_id && !slot_header->is_free) {
                return memory_.get() + page_id * PAGE_SIZE + slot_header->data_offset;
            }

            offset += sizeof(SlotHeader) + slot_header->size;
        }

        return Result<void*>("Slot not found: page=" + std::to_string(page_id) +
                              ", slot=" + std::to_string(slot_id));
    }

    // Get statistics
    MemoryStats getStats() const {
        std::shared_lock lock(mutex_);
        return {
            pool_size_,
            used_size_,
            pool_size_ - used_size_,
            pages_.size(),
            free_pages_.size(),
            allocation_count_,
            deallocation_count_
        };
    }

    // Get all dirty pages
    std::vector<PageId> getDirtyPages() const {
        std::shared_lock lock(mutex_);
        std::vector<PageId> dirty_pages;

        for (const auto& [page_id, header] : pages_) {
            if (header->is_dirty) {
                dirty_pages.push_back(page_id);
            }
        }

        return dirty_pages;
    }

private:
    SpaceType space_type_;
    size_t pool_size_;
    size_t used_size_;
    size_t allocation_count_;
    size_t deallocation_count_;
    std::unique_ptr<std::byte[]> memory_;

    std::unordered_map<PageId, PageHeader*> pages_;
    std::vector<PageId> free_pages_;

    mutable std::shared_mutex mutex_;
};

// Main Memory Manager Implementation
class MemManager : public IMemManager {
public:
    MemManager() = default;

    ~MemManager() override {
        if (initialized_) {
            close();
        }
    }

    // ===== Initialization =====

    Result<void> initialize(size_t pool_size) override {
        if (initialized_) {
            return Result<void>("Memory manager already initialized");
        }

        if (pool_size < MIN_POOL_SIZE) {
            return Result<void>("Pool size too small: minimum=" +
                                   std::to_string(MIN_POOL_SIZE));
        }

        if (pool_size > MAX_POOL_SIZE) {
            return Result<void>("Pool size too large: maximum=" +
                                   std::to_string(MAX_POOL_SIZE));
        }

        // Divide pool size among spaces
        // Control: 5%, Table: 50%, Index: 25%, Undo: 10%, Redo: 10%
        size_t control_size = pool_size * 0.05;
        size_t table_size = pool_size * 0.50;
        size_t index_size = pool_size * 0.25;
        size_t undo_size = pool_size * 0.10;
        size_t redo_size = pool_size * 0.10;

        // Create space pools
        spaces_[SpaceType::Control] = std::make_unique<SpacePool>(SpaceType::Control,
                                                                   control_size);
        spaces_[SpaceType::Table] = std::make_unique<SpacePool>(SpaceType::Table,
                                                                 table_size);
        spaces_[SpaceType::Index] = std::make_unique<SpacePool>(SpaceType::Index,
                                                                 index_size);
        spaces_[SpaceType::Undo] = std::make_unique<SpacePool>(SpaceType::Undo,
                                                               undo_size);
        spaces_[SpaceType::Redo] = std::make_unique<SpacePool>(SpaceType::Redo,
                                                               redo_size);

        total_size_ = pool_size;
        initialized_ = true;

        SMDB_LOG_INFO("Memory manager initialized: total_size=" + std::to_string(pool_size));

        return {};
    }

    Result<void> open() override {
        if (!initialized_) {
            return Result<void>("Memory manager not initialized");
        }

        SMDB_LOG_INFO("Memory manager opened");
        return {};
    }

    Result<void> close() override {
        if (!initialized_) {
            return Result<void>("Memory manager not initialized");
        }

        spaces_.clear();
        page_to_space_.clear();
        initialized_ = false;

        SMDB_LOG_INFO("Memory manager closed");
        return {};
    }

    // ===== Page Management =====

    Result<PageId> allocatePage(SpaceType space) override {
        if (!initialized_) {
            return Result<PageId>("Memory manager not initialized");
        }

        auto it = spaces_.find(space);
        if (it == spaces_.end()) {
            return Result<PageId>("Invalid space type: " +
                                   std::to_string(static_cast<int>(space)));
        }

        auto result = it->second->allocatePage();
        if (!result) {
            return result;
        }

        PageId page_id = *result;
        page_to_space_[page_id] = space;

        SMDB_LOG_INFO("Allocated page: id=" + std::to_string(page_id) +
                     ", space=" + std::to_string(static_cast<int>(space)));

        return page_id;
    }

    Result<void> freePage(PageId page_id) override {
        if (!initialized_) {
            return Result<void>("Memory manager not initialized");
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return Result<void>("Page not found: " + std::to_string(page_id));
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return Result<void>("Invalid space type");
        }

        auto result = space_it->second->freePage(page_id);
        if (!result) {
            return result;
        }

        page_to_space_.erase(it);

        SMDB_LOG_INFO("Freed page: id=" + std::to_string(page_id));

        return {};
    }

    Result<void*> getPage(PageId page_id) override {
        if (!initialized_) {
            return Result<void*>("Memory manager not initialized");
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return Result<void*>("Page not found: " + std::to_string(page_id));
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return Result<void*>("Invalid space type");
        }

        return space_it->second->getPage(page_id);
    }

    void markDirty(PageId page_id, SCN scn) override {
        if (!initialized_) {
            return;
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return;
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return;
        }

        auto result = space_it->second->getPageHeader(page_id);
        if (result) {
            auto* header = *result;
            header->is_dirty = true;
            header->last_scn = scn;
        }
    }

    Result<PageInfo> getPageInfo(PageId page_id) override {
        if (!initialized_) {
            return Result<PageInfo>("Memory manager not initialized");
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return Result<PageInfo>("Page not found: " + std::to_string(page_id));
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return Result<PageInfo>("Invalid space type");
        }

        auto result = space_it->second->getPageHeader(page_id);
        if (!result) {
            return Result<PageInfo>(result.error());
        }

        auto* header = *result;

        return PageInfo{
            header->page_id,
            header->space,
            header->size,
            header->used,
            header->is_dirty,
            header->last_scn,
            MemoryPosition()  // TODO: Implement next_page tracking
        };
    }

    // ===== Slot Management =====

    Result<SlotId> allocateSlot(PageId page_id, size_t size) override {
        if (!initialized_) {
            return Result<SlotId>("Memory manager not initialized");
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return Result<SlotId>("Page not found: " + std::to_string(page_id));
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return Result<SlotId>("Invalid space type");
        }

        return space_it->second->allocateSlot(page_id, size);
    }

    Result<void> freeSlot(PageId page_id, SlotId slot_id) override {
        // TODO: Implement slot freeing
        return Result<void>("Not implemented yet");
    }

    Result<void*> getSlot(PageId page_id, SlotId slot_id) override {
        if (!initialized_) {
            return Result<void*>("Memory manager not initialized");
        }

        auto it = page_to_space_.find(page_id);
        if (it == page_to_space_.end()) {
            return Result<void*>("Page not found: " + std::to_string(page_id));
        }

        SpaceType space = it->second;
        auto space_it = spaces_.find(space);
        if (space_it == spaces_.end()) {
            return Result<void*>("Invalid space type");
        }

        return space_it->second->getSlot(page_id, slot_id);
    }

    // ===== Address Translation =====

    void* toPhysicalAddr(const MemoryPosition& pos) override {
        if (!initialized_) {
            return nullptr;
        }

        auto it = spaces_.find(pos.space);
        if (it == spaces_.end()) {
            return nullptr;
        }

        // Calculate physical address based on offset
        // For now, this is a simplified implementation
        // In a real system, this would need to account for the actual memory layout
        return nullptr;  // TODO: Implement proper address translation
    }

    MemoryPosition fromPhysicalAddr(void* addr) override {
        // TODO: Implement reverse address translation
        return MemoryPosition();
    }

    // ===== Direct Allocation =====

    Result<AllocationResult> allocate(SpaceType space, size_t size) override {
        // For now, delegate to page allocation
        auto page_result = allocatePage(space);
        if (!page_result) {
            return Result<AllocationResult>(page_result.error());
        }

        PageId page_id = *page_result;
        auto ptr_result = getPage(page_id);
        if (!ptr_result) {
            return Result<AllocationResult>(ptr_result.error());
        }

        return AllocationResult{
            MemoryPosition(space, page_id * PAGE_SIZE),
            *ptr_result,
            PAGE_SIZE
        };
    }

    Result<void> free(const MemoryPosition& pos) override {
        // TODO: Implement direct free
        return Result<void>("Not implemented yet");
    }

    // ===== Statistics =====

    MemoryStats getStats() const override {
        MemoryStats total{};
        std::shared_lock lock(mutex_);

        for (const auto& [type, pool] : spaces_) {
            auto stats = pool->getStats();
            total.total_size += stats.total_size;
            total.used_size += stats.used_size;
            total.free_size += stats.free_size;
            total.page_count += stats.page_count;
            total.free_page_count += stats.free_page_count;
            total.allocation_count += stats.allocation_count;
            total.deallocation_count += stats.deallocation_count;
        }

        return total;
    }

    MemoryStats getSpaceStats(SpaceType space) const override {
        auto it = spaces_.find(space);
        if (it == spaces_.end()) {
            return MemoryStats{};
        }

        return it->second->getStats();
    }

    // ===== Checkpoint & Recovery =====

    Result<size_t> flushDirtyPages(SCN scn) override {
        if (!initialized_) {
            return Result<size_t>("Memory manager not initialized");
        }

        size_t flushed_count = 0;

        for (auto& [type, pool] : spaces_) {
            auto dirty_pages = pool->getDirtyPages();
            for (PageId page_id : dirty_pages) {
                // TODO: Implement actual disk flush
                flushed_count++;

                SMDB_LOG_DEBUG("Flushed dirty page: id=" + std::to_string(page_id) +
                              ", scn=" + std::to_string(scn));
            }
        }

        SMDB_LOG_INFO("Flushed dirty pages: count=" + std::to_string(flushed_count) +
                     ", scn=" + std::to_string(scn));

        return flushed_count;
    }

    std::vector<PageId> getDirtyPages() const override {
        std::vector<PageId> all_dirty_pages;

        for (const auto& [type, pool] : spaces_) {
            auto dirty_pages = pool->getDirtyPages();
            all_dirty_pages.insert(all_dirty_pages.end(),
                                  dirty_pages.begin(),
                                  dirty_pages.end());
        }

        return all_dirty_pages;
    }

    void clearDirtyFlags(const std::vector<PageId>& page_ids) override {
        if (!initialized_) {
            return;
        }

        for (PageId page_id : page_ids) {
            auto it = page_to_space_.find(page_id);
            if (it == page_to_space_.end()) {
                continue;
            }

            SpaceType space = it->second;
            auto space_it = spaces_.find(space);
            if (space_it == spaces_.end()) {
                continue;
            }

            auto result = space_it->second->getPageHeader(page_id);
            if (result) {
                auto* header = *result;
                header->is_dirty = false;
            }
        }
    }

private:
    bool initialized_ = false;
    size_t total_size_ = 0;

    std::unordered_map<SpaceType, std::unique_ptr<SpacePool>> spaces_;
    std::unordered_map<PageId, SpaceType> page_to_space_;

    mutable std::shared_mutex mutex_;
};

// Factory Implementation
Result<std::unique_ptr<IMemManager>> MemManagerFactory::create() {
    try {
        return std::make_unique<MemManager>();
    } catch (const std::exception& e) {
        return Result<size_t>(std::string("Failed to create memory manager: ") +
                              e.what());
    }
}

} // namespace smdb
