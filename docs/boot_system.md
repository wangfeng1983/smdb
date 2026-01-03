# SMDB 启动系统设计

## 概述

SMDB的启动系统是一个现代化的、不依赖任何私有库的应用框架。它取代了MDB中使用的`appfrm/Application`私有框架，完全使用C++21标准和开源库实现。

## 设计目标

1. **无私有依赖**: 仅使用开源库，避免供应商锁定
2. **现代化**: 利用C++21特性（如std::expected, std::source_location等）
3. **灵活性**: 支持多种应用模式（Server, Console, Tool）
4. **可测试性**: 清晰的生命周期钩子，便于单元测试
5. **可扩展性**: 易于添加新的应用类型

## 架构设计

### 核心组件

```
┌─────────────────────────────────────────┐
│           IApplication (interface)       │
├─────────────────────────────────────────┤
│         ApplicationBase (base class)     │
│  - run()                                 │
│  - stop()                                │
│  - Lifecycle hooks                       │
├─────────────────────────────────────────┤
│    ┌──────────────┬──────────────────┐   │
│    │ Server       │  Console         │   │
│    │ Application  │  Application     │   │
│    ├──────────────┼──────────────────┤   │
│    │ Tool         │  (Custom)        │   │
│    │ Application  │  Applications    │   │
│    └──────────────┴──────────────────┘   │
└─────────────────────────────────────────┘
```

### 辅助组件

1. **SimpleLogger**: 轻量级日志系统
2. **CommandLineParser**: 命令行参数解析
3. **ConfigManager**: 配置文件管理（JSON格式）
4. **CliApp**: CLI应用框架

## 应用类型

### 1. ServerApplication (服务器应用)

用于长时间运行的服务器/守护进程。

**特点：**
- 作为守护进程运行
- 监听网络连接
- 处理客户端请求
- 优雅关闭支持

**示例：**
```cpp
class SmdbServerApp : public ServerApplication {
protected:
    Result<void> onInitialize() override {
        // 初始化数据库
    }

    Result<void> onStartServer() override {
        // 启动网络监听
    }

    Result<void> onLoop() override {
        // 处理客户端连接
    }

    Result<void> onStopServer() override {
        // 停止服务器
    }
};
```

**使用：**
```bash
smdb-server --port 9527 --data-dir /data --daemon
```

### 2. ConsoleApplication (控制台应用)

用于交互式命令行工具。

**特点：**
- 交互式命令提示
- 命令历史
- 实时反馈

**示例：**
```cpp
class SmdbConsoleApp : public ConsoleApplication {
protected:
    void showWelcome() override {
        std::cout << "SMDB Console v0.1.0\n";
    }

    std::string getPrompt() const override {
        return "smdb";
    }

    bool processCommand(const std::string& cmd) override {
        if (cmd == "exit") return false;
        // 处理命令
        return true;
    }
};
```

**使用：**
```bash
smdb-console
smdb> connect localhost:9527
smdb> CREATE TABLE users (id INT, name STRING);
smdb> exit
```

### 3. ToolApplication (工具应用)

用于一次性执行的工具（备份、恢复等）。

**特点：**
- 无交互循环
- 执行单个任务
- 执行完成后退出

**示例：**
```cpp
class SmdbBackupApp : public ToolApplication {
protected:
    int executeTool() override {
        // 执行备份
        return 0; // 返回码
    }
};
```

**使用：**
```bash
smdb-backup --output /backup/smdb_backup.db
```

## 生命周期

完整的生命周期流程：

```
Created
    ↓
Initializing → onInitialize()
    ↓
Initialized
    ↓
Starting → onBeforeLoop()
    ↓
Running → onLoop() [repeated]
    ↓
Stopping → onAfterLoop()
    ↓
Stopped
    ↓
Cleanup → onCleanup()
```

### 生命周期钩子

| 钩子 | 用途 | 返回值 |
|------|------|--------|
| `onInitialize()` | 初始化资源 | Result<void> |
| `onBeforeLoop()` | 主循环前准备 | Result<void> |
| `onLoop()` | 主循环体 | Result<void> |
| `onAfterLoop()` | 主循环后清理 | Result<void> |
| `onCleanup()` | 最终清理 | Result<void> |
| `onSignal(int)` | 信号处理 | void |

## 配置系统

### 命令行参数

支持短选项和长选项：

```bash
# 短选项
smdb-server -p 9527 -v

# 长选项
smdb-server --port 9527 --verbose

# 组合
smdb-server -p 9527 -d --data-dir /data
```

### 配置文件

使用JSON格式配置：

```json
{
  "db": {
    "name": "smdb",
    "memory_pool_size": 1073741824,
    "data_dir": "./data",
    "enable_persistence": true,
    "enable_recovery": true,
    "checkpoint_interval": 60,
    "max_connections": 100
  },
  "server": {
    "host": "0.0.0.0",
    "port": 9527,
    "max_connections": 100,
    "idle_timeout": 300
  },
  "logging": {
    "level": "info",
    "file": "/var/log/smdb/server.log"
  }
}
```

### 环境变量

支持通过环境变量配置：

```bash
export SMDB_DB_NAME=smdb
export SMDB_DB_DATA_DIR=/data
export SMDB_SERVER_PORT=9527
export SMDB_LOG_LEVEL=debug
```

## 日志系统

### 日志级别

- `Trace`: 最详细的跟踪信息
- `Debug`: 调试信息
- `Info`: 一般信息（默认）
- `Warning`: 警告信息
- `Error`: 错误信息
- `Critical`: 严重错误
- `Off`: 关闭日志

### 使用示例

```cpp
#include "smdb/utils/logger.h"

// 简单日志
SMDB_LOG_INFO("Server started on port " + std::to_string(port));
SMDB_LOG_ERROR("Failed to connect: " + error_message);
SMDB_LOG_DEBUG("Processing request: " + request.to_string());

// 带位置信息的日志（仅Debug模式）
#ifndef NDEBUG
SMDB_LOG_DEBUG("Variable value: " + std::to_string(value));
#endif
```

### 日志输出

```
[2024-01-03 12:34:56] [INFO ] [main.cpp:42] Server started on port 9527
[2024-01-03 12:34:57] [ERROR] [connection.cpp:15] Failed to connect: Connection refused
```

## 与MDB的对比

| 特性 | MDB | SMDB |
|------|-----|------|
| 基类 | `Application` (私有) | `IApplication` (开源) |
| 配置 | 私有格式 | JSON |
| 日志 | 私有Logger | SimpleLogger / 可集成spdlog |
| CLI | 私有框架 | CLI11兼容 / 自包含 |
| 信号处理 | 框架内置 | 明确的钩子 |
| 生命周期 | 固定流程 | 灵活的钩子系统 |

## 扩展性

### 添加新的应用类型

```cpp
class CustomApplication : public ApplicationBase {
protected:
    Result<void> onInitialize() override {
        // 自定义初始化
    }

    Result<void> onLoop() override {
        // 自定义循环逻辑
    }
};
```

### 集成第三方库

#### 使用spdlog替代SimpleLogger

```cpp
#include <spdlog/spdlog.h>

void integrateSpdlog() {
    auto logger = spdlog::basic_logger_mt("smdb", "logs/smdb.log");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);
}
```

#### 使用nlohmann/json替代SimpleJsonParser

```cpp
#include <nlohmann/json.hpp>

Result<void> loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    nlohmann::json config;
    file >> config;

    // 解析配置
    std::string db_name = config["db"]["name"];
    // ...
}
```

#### 使用CLI11进行参数解析

```cpp
#include <CLI/CLI.hpp>

int main(int argc, char* argv[]) {
    CLI::App app("SMDB Server");

    int port = 9527;
    app.add_option("-p,--port", port, "Server port");

    std::string config_file;
    app.add_option("-c,--config", config_file, "Config file");

    CLI11_PARSE(app, argc, argv);
}
```

## 最佳实践

### 1. 错误处理

始终使用`Result<T>`返回错误：

```cpp
Result<void> onInitialize() override {
    auto result = someOperation();
    if (!result) {
        return std::unexpected("Failed: " + result.error());
    }
    return {};
}
```

### 2. 日志记录

在关键点记录日志：

```cpp
Result<void> onInitialize() override {
    SMDB_LOG_INFO("Initializing application...");

    auto result = initializeDatabase();
    if (!result) {
        SMDB_LOG_ERROR("Database initialization failed: " + result.error());
        return result;
    }

    SMDB_LOG_INFO("Initialization completed successfully");
    return {};
}
```

### 3. 资源清理

在`onCleanup()`中确保资源释放：

```cpp
Result<void> onCleanup() override {
    SMDB_LOG_INFO("Cleaning up resources...");

    if (database_) {
        database_->shutdown();
        database_.reset();
    }

    if (server_socket_ != -1) {
        close(server_socket_);
        server_socket_ = -1;
    }

    SMDB_LOG_INFO("Cleanup completed");
    return {};
}
```

### 4. 信号处理

正确处理信号以实现优雅关闭：

```cpp
void onSignal(int signal) override {
    SMDB_LOG_INFO("Received signal: " + std::to_string(signal));

    // 保存状态
    saveState();

    // 停止接受新连接
    stopAccepting();

    // 调用基类设置停止标志
    ServerApplication::onSignal(signal);
}
```

## 总结

SMDB的启动系统提供了一个现代化、灵活、无私有依赖的应用框架。它：

- ✅ 完全开源，无供应商锁定
- ✅ 使用C++21现代特性
- ✅ 支持多种应用模式
- ✅ 清晰的生命周期管理
- ✅ 易于测试和扩展
- ✅ 可集成第三方库（spdlog, CLI11, nlohmann/json等）

这为SMDB项目的后续开发提供了坚实的基础。
