# SMDB 项目继续开发指南

> **📅 最后更新**: 2026-01-03
> **📍 当前分支**: `develop`
> **🎯 当前阶段**: 架构设计完成，准备开始实现核心模块

---

## 📋 快速开始

### 第一步：了解当前状态

```bash
# 切换到开发分支
cd D:\2026\claude\smdb
git checkout develop

# 查看项目结构
tree /F /A

# 或使用ls（如果你有Git Bash）
ls -la
```

### 第二步：阅读关键文档

按优先级阅读：

1. **本项目文件** ← 你在这里
2. `docs/project_status.md` - 项目总体状态
3. `docs/architecture.md` - 架构设计详解
4. `docs/implementation_plan.md` - 12周实施计划
5. `docs/boot_system.md` - 启动系统说明

---

## ✅ 已完成工作（2026-01-03）

### 1. 架构设计阶段 ✅

- ✅ 升级到C++21标准
- ✅ 设计完整分层架构
- ✅ 定义7个核心模块接口
- ✅ 设计启动系统（无私有依赖）

### 2. 接口定义 ✅

**核心接口位置**：

```
include/smdb/
├── core/
│   ├── database.h          # 数据库核心接口
│   └── table.h             # 表接口
├── storage/
│   └── mem_manager.h       # 内存管理接口
├── index/
│   └── index.h             # 索引接口
├── transaction/
│   └── transaction.h       # 事务接口
├── concurrency/
│   └── lock_manager.h      # 锁管理接口
├── recovery/
│   └── recovery_manager.h  # 恢复管理接口
└── utils/
    ├── types.h             # 核心类型定义 ⭐ 重要
    ├── application.h       # 启动框架
    ├── logger.h            # 日志系统
    ├── cli.h               # 命令行解析
    └── config.h            # 配置管理
```

### 3. 启动系统实现 ✅

```
src/utils/
├── application.cpp         # 应用框架实现
├── cli.cpp                 # 命令行解析实现
└── config.cpp              # 配置管理实现

src/app/
└── smdb_server.cpp         # 服务器应用示例
```

### 4. 文档完成 ✅

```
docs/
├── architecture.md         # 架构设计 ⭐ 必读
├── migration_summary.md    # MDB迁移总结
├── implementation_plan.md  # 实施计划 ⭐ 必读
├── boot_system.md          # 启动系统详解
└── project_status.md       # 项目状态
```

### 5. Git仓库 ✅

- **仓库**: https://github.com/wangfeng1983/smdb
- **本地路径**: `D:\2026\claude\smdb`
- **当前分支**: `develop`
- **提交记录**: 2个commit（初始提交 + 状态文档）

---

## 🎯 下一步工作（按优先级）

### 立即开始：实现MemManager（内存管理器）

**这是下一个要实现的模块**，原因：
1. 其他模块都依赖它
2. 是基础设施的核心
3. 相对独立，便于测试

**实现文件**：
```
src/storage/mem_manager.cpp  # 需要创建
include/smdb/storage/mem_manager.h  # 接口已定义
```

**核心功能**（从`mem_manager.h`接口）：
```cpp
class IMemManager {
    // 初始化
    virtual Result<void> initialize(size_t pool_size) = 0;
    virtual Result<void> open() = 0;
    virtual Result<void> close() = 0;

    // 页管理
    virtual Result<PageId> allocatePage(SpaceType space) = 0;
    virtual Result<void> freePage(PageId page_id) = 0;
    virtual Result<void*> getPage(PageId page_id) = 0;

    // 槽管理
    virtual Result<SlotId> allocateSlot(PageId page_id, size_t size) = 0;
    virtual Result<void*> getSlot(PageId page_id, SlotId slot_id) = 0;

    // 地址转换
    virtual void* toPhysicalAddr(const MemoryPosition& pos) = 0;
    virtual MemoryPosition fromPhysicalAddr(void* addr) = 0;

    // 统计
    virtual MemoryStats getStats() const = 0;
};
```

**参考MDB实现**：
```
mdb/MemManager.h
mdb/memMgr/ControlFile.cpp
mdb/memMgr/DataPagesMgr.cpp
```

---

## 📖 实现指南

### 如何开始实现一个模块

#### 1. 创建实现类

```cpp
// src/storage/mem_manager.cpp
#include "smdb/storage/mem_manager.h"
#include "smdb/utils/logger.h"

namespace smdb {

class MemManager : public IMemManager {
public:
    MemManager() = default;
    ~MemManager() override = default;

    Result<void> initialize(size_t pool_size) override {
        SMDB_LOG_INFO("Initializing memory manager with pool size: " +
                     std::to_string(pool_size));

        // TODO: 实现初始化逻辑

        return {};
    }

    // 实现其他接口方法...

private:
    // 私有成员
    size_t pool_size_;
    std::vector<std::byte> memory_pool_;
    // ...
};

// 工厂函数
Result<std::unique_ptr<IMemManager>> MemManagerFactory::create() {
    return std::make_unique<MemManager>();
}

} // namespace smdb
```

#### 2. 更新CMakeLists.txt

```cmake
# src/CMakeLists.txt 已配置，会自动扫描src/storage/*.cpp
```

#### 3. 创建单元测试

```cpp
// tests/unit/test_mem_manager.cpp
#include "smdb/storage/mem_manager.h"
#include <gtest/gtest.h>

namespace smdb {
namespace test {

TEST(MemManagerTest, Initialize) {
    auto mgr = MemManagerFactory::create();
    ASSERT_TRUE(mgr);

    auto result = (*mgr)->initialize(1024 * 1024); // 1MB
    EXPECT_TRUE(result);

    auto stats = (*mgr)->getStats();
    EXPECT_GT(stats.total_size, 0);
}

} // namespace test
} // namespace smdb
```

#### 4. 构建和测试

```bash
cd smdb
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# 运行测试
ctest --output-on-failure
```

---

## 🔍 关键设计概念

### 1. 内存空间类型（SpaceType）

```cpp
enum class SpaceType : uint8_t {
    Control = 0,    // 控制信息
    Table = 1,      // 表数据
    Index = 2,      // 索引数据
    Undo = 3,       // Undo日志
    Redo = 4        // Redo日志
};
```

**MDB对应**：不同空间用于不同目的，隔离管理。

### 2. 页式存储

```cpp
constexpr size_t PAGE_SIZE = 8 * 1024; // 8KB

struct PageInfo {
    PageId page_id;
    SpaceType space;
    size_t size;
    size_t used;
    bool is_dirty;
    SCN last_scn;
    MemoryPosition next_page;
};
```

**MDB对应**：与MDB的页管理类似。

### 3. 地址转换

```cpp
struct MemoryPosition {
    SpaceType space;
    uint64_t offset;
};

// 虚拟地址 → 物理地址
void* toPhysicalAddr(const MemoryPosition& pos);

// 物理地址 → 虚拟地址
MemoryPosition fromPhysicalAddr(void* addr);
```

**MDB对应**：类似MDB的ShmPosition和地址转换。

---

## 📚 MDB参考代码位置

当你实现时，可以参考MDB的对应代码：

| SMDB模块 | MDB参考文件 | 位置 |
|---------|------------|------|
| MemManager | MemManager.h/cpp | mdb/ |
| | ControlFile.cpp | mdb/memMgr/ |
| | DataPagesMgr.cpp | mdb/memMgr/ |
| Table | Table.h/cpp | mdb/ |
| Index | Index*.h/cpp | mdb/ |
| | BTree/ | mdb/BTree/ |
| Transaction | TransactionMgr.cpp | mdb/lockMgr/ |
| LockManager | LockMgr*.cpp | mdb/lockMgr/ |
| Recovery | Recovery*.h/cpp | mdb/ |

---

## 🛠️ 开发工作流

### 日常开发流程

```bash
# 1. 切换到develop并更新
git checkout develop
git pull origin develop

# 2. 创建功能分支
git checkout -b feature/mem-manager

# 3. 实现代码
# 编辑 src/storage/mem_manager.cpp

# 4. 提交
git add .
git commit -m "Implement MemManager basic functionality"

# 5. 推送
git push -u origin feature/mem-manager

# 6. 如果需要，合并回develop
git checkout develop
git merge feature/mem-manager
git push origin develop
```

### 与Claude协作时

**当你要继续开发时，告诉Claude：**

```
我正在继续SMDB项目的开发。

当前状态：
- 项目位置：D:\2026\claude\smdb
- 当前分支：develop
- 上次工作：完成了架构设计和启动系统
- 下一步：实现MemManager（内存管理器）

请参考：
- docs/architecture.md - 架构设计
- docs/implementation_plan.md - 实施计划
- include/smdb/storage/mem_manager.h - 接口定义
- mdb/MemManager.h - MDB参考实现

我想要实现的具体功能是：
[这里描述你想实现的功能]
```

---

## 📊 项目进度跟踪

### 完成度

```
总体进度: ████░░░░░░ 20%

阶段1: 架构设计        ████████████ 100% ✅
阶段2: 基础设施       ██░░░░░░░░░░  20%  ← 你在这里
├─ 启动系统          ████████████ 100% ✅
├─ MemManager       █░░░░░░░░░░░   0%  ⭐ 下一步
├─ LockManager      ░░░░░░░░░░░░   0%
└─ LatchManager     ░░░░░░░░░░░░   0%

阶段3: 数据存储       ░░░░░░░░░░░░   0%
阶段4: 事务支持       ░░░░░░░░░░░░   0%
阶段5: 持久化         ░░░░░░░░░░░░   0%
阶段6: 集成           ░░░░░░░░░░░░   0%
```

### TODO清单

- [ ] 实现 MemManager
- [ ] 实现 LockManager
- [ ] 实现 LatchManager
- [ ] 实现 Table
- [ ] 实现 HashIndex
- [ ] 实现 BTreeIndex
- [ ] 实现 TransactionManager
- [ ] 实现 RedoLog
- [ ] 实现 UndoLog
- [ ] 实现 RecoveryManager
- [ ] 实现 Database核心类
- [ ] 添加完整单元测试
- [ ] 性能优化
- [ ] 编写用户手册

---

## 🐛 常见问题和解决方案

### Q1: 编译错误 "expected ';' after 'return'"

**原因**: C++21的std::expected用法

**解决**:
```cpp
// ❌ 错误
return {};

// ✅ 正确（成功）
return {};

// ✅ 正确（错误）
return std::unexpected("Error message");
```

### Q2: 找不到头文件

**解决**: 确保include目录正确
```cmake
include_directories(${PROJECT_SOURCE_DIR}/include)
```

### Q3: 链接错误

**解决**: 检查CMakeLists.txt中的源文件是否包含
```bash
# 查看实际编译的文件
cd build
make VERBOSE=1
```

---

## 🔗 重要链接

- **GitHub仓库**: https://github.com/wangfeng1983/smdb
- **开发分支**: https://github.com/wangfeng1983/smdb/tree/develop
- **架构文档**: `docs/architecture.md`
- **实施计划**: `docs/implementation_plan.md`

---

## 💡 提示

### 给Claude的最佳提示方式

**详细版**（推荐）：
```
我正在实现SMDB的MemManager模块。

项目位置：D:\2026\claude\smdb
参考：docs/architecture.md, include/smdb/storage/mem_manager.h

我想实现页分配功能：
- 使用buddy分配器算法
- 8KB页大小
- 支持不同空间类型的分配
- 参考mdb/MemManager.h的实现

请帮我：
1. 实现allocatePage()方法
2. 添加必要的辅助数据结构
3. 添加错误处理
```

**简洁版**：
```
继续SMDB项目的开发，当前在实现MemManager模块。
请参考mdb/MemManager.h和include/smdb/storage/mem_manager.h，
实现页分配功能。
```

### 查看当前上下文

```bash
# 查看最近的提交
git log --oneline -5

# 查看当前分支
git branch

# 查看未提交的更改
git status
```

---

## 📞 获取帮助

如果遇到问题：

1. **查看文档**: `docs/` 目录下的所有文档
2. **参考MDB**: `mdb/` 目录下的对应实现
3. **查看接口**: `include/smdb/` 下的接口定义
4. **运行测试**: `build/` 目录下的测试程序

---

## 🎓 学习资源

- **C++21**: https://en.cppreference.com/w/cpp/21
- **ARIES论文**: https://www.microsoft.com/en-us/research/publication/aries-a-transaction-recovery-method-supporting-fine-granularity-locking-and-partial-rollbacks-using-write-ahead-logging/
- **B+树**: https://en.wikipedia.org/wiki/B%2B_tree

---

**记住**：你正在将MDB的成熟架构用现代C++21重新实现，保持核心思想的同时，使用更现代、更安全、更高效的方式。

**下一个目标**：实现MemManager的initialize()和allocatePage()方法 💪

---

*祝开发顺利！🚀*
