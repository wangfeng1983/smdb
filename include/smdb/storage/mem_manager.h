#ifndef SMDB_STORAGE_MEM_MANAGER_H
#define SMDB_STORAGE_MEM_MANAGER_H

#include "smdb/utils/types.h"
#include <memory>
#include <vector>
#include <cstddef>

namespace smdb {

/**
 * @brief Memory space types
 */
enum class SpaceType : uint8_t {
    Control = 0,    // Control information
    Table = 1,      // Table data
    Index = 2,      // Index data
    Undo = 3,       // Undo logs
    Redo = 4        // Redo logs
};

/**
 * @brief Memory position (similar to MDB's ShmPosition)
 */
struct MemoryPosition {
    SpaceType space;
    uint64_t offset;

    constexpr MemoryPosition() : space(SpaceType::Control), offset(0) {}
    constexpr MemoryPosition(SpaceType s, uint64_t o) : space(s), offset(o) {}

    bool operator==(const MemoryPosition& other) const {
        return space == other.space && offset == other.offset;
    }

    bool operator!=(const MemoryPosition& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Page information
 */
struct PageInfo {
    PageId page_id;
    SpaceType space;
    size_t size;
    size_t used;
    bool is_dirty;
    SCN last_scn;
    MemoryPosition next_page;
};

/**
 * @brief Slot information (within a page)
 */
struct SlotInfo {
    SlotId slot_id;
    size_t offset;
    size_t size;
    bool is_free;
    bool is_deleted;
    SCN create_scn;
    SCN delete_scn;
};

/**
 * @brief Memory allocation result
 */
struct AllocationResult {
    MemoryPosition position;
    void* pointer;
    size_t actual_size;
};

/**
 * @brief Memory pool statistics
 */
struct MemoryStats {
    size_t total_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;
    size_t page_count = 0;
    size_t free_page_count = 0;
    size_t allocation_count = 0;
    size_t deallocation_count = 0;
};

/**
 * @brief Memory Manager Interface
 *
 * Manages in-memory storage with page-based allocation
 */
class IMemManager {
public:
    virtual ~IMemManager() = default;

    // ===== Initialization =====

    /**
     * @brief Initialize memory manager
     * @param pool_size Total memory pool size
     * @return Result<void> Success or error message
     */
    virtual Result<void> initialize(size_t pool_size) = 0;

    /**
     * @brief Open existing memory
     * @return Result<void> Success or error message
     */
    virtual Result<void> open() = 0;

    /**
     * @brief Close memory manager
     * @return Result<void> Success or error message
     */
    virtual Result<void> close() = 0;

    // ===== Page Management =====

    /**
     * @brief Allocate a new page
     * @param space Memory space
     * @return Result<PageId> Allocated page ID or error
     */
    virtual Result<PageId> allocatePage(SpaceType space) = 0;

    /**
     * @brief Free a page
     * @param page_id Page ID
     * @return Result<void> Success or error message
     */
    virtual Result<void> freePage(PageId page_id) = 0;

    /**
     * @brief Get a page
     * @param page_id Page ID
     * @return Result<void*> Page pointer or error
     */
    virtual Result<void*> getPage(PageId page_id) = 0;

    /**
     * @brief Mark page as dirty
     * @param page_id Page ID
     * @param scn Current SCN
     */
    virtual void markDirty(PageId page_id, SCN scn) = 0;

    /**
     * @brief Get page information
     * @param page_id Page ID
     * @return Result<PageInfo> Page info or error
     */
    virtual Result<PageInfo> getPageInfo(PageId page_id) = 0;

    // ===== Slot Management =====

    /**
     * @brief Allocate a slot in a page
     * @param page_id Page ID
     * @param size Slot size
     * @return Result<SlotId> Allocated slot ID or error
     */
    virtual Result<SlotId> allocateSlot(PageId page_id, size_t size) = 0;

    /**
     * @brief Free a slot
     * @param page_id Page ID
     * @param slot_id Slot ID
     * @return Result<void> Success or error message
     */
    virtual Result<void> freeSlot(PageId page_id, SlotId slot_id) = 0;

    /**
     * @brief Get slot data
     * @param page_id Page ID
     * @param slot_id Slot ID
     * @return Result<void*> Slot pointer or error
     */
    virtual Result<void*> getSlot(PageId page_id, SlotId slot_id) = 0;

    // ===== Address Translation =====

    /**
     * @brief Convert position to physical address
     * @param pos Memory position
     * @return void* Physical address
     */
    virtual void* toPhysicalAddr(const MemoryPosition& pos) = 0;

    /**
     * @brief Convert physical address to position
     * @param addr Physical address
     * @return MemoryPosition Memory position
     */
    virtual MemoryPosition fromPhysicalAddr(void* addr) = 0;

    // ===== Direct Allocation =====

    /**
     * @brief Allocate raw memory
     * @param space Memory space
     * @param size Size to allocate
     * @return Result<AllocationResult> Allocation result or error
     */
    virtual Result<AllocationResult> allocate(SpaceType space, size_t size) = 0;

    /**
     * @brief Free raw memory
     * @param pos Memory position
     * @return Result<void> Success or error message
     */
    virtual Result<void> free(const MemoryPosition& pos) = 0;

    // ===== Statistics =====

    /**
     * @brief Get memory statistics
     * @return Memory statistics
     */
    virtual MemoryStats getStats() const = 0;

    /**
     * @brief Get statistics for a specific space
     * @param space Memory space
     * @return Memory statistics for space
     */
    virtual MemoryStats getSpaceStats(SpaceType space) const = 0;

    // ===== Checkpoint & Recovery =====

    /**
     * @brief Flush dirty pages to disk
     * @param scn SCN to flush up to
     * @return Result<size_t> Number of pages flushed or error
     */
    virtual Result<size_t> flushDirtyPages(SCN scn) = 0;

    /**
     * @brief Get all dirty pages
     * @return Vector of dirty page IDs
     */
    virtual std::vector<PageId> getDirtyPages() const = 0;

    /**
     * @brief Clear dirty flag for pages
     * @param page_ids Page IDs
     */
    virtual void clearDirtyFlags(const std::vector<PageId>& page_ids) = 0;
};

/**
 * @brief Memory Manager Factory
 */
class MemManagerFactory {
public:
    /**
     * @brief Create a memory manager
     * @param config Configuration
     * @return Result<std::unique_ptr<IMemManager>> Memory manager or error
     */
    static Result<std::unique_ptr<IMemManager>> create();
};

} // namespace smdb

#endif // SMDB_STORAGE_MEM_MANAGER_H
