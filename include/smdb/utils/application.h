#ifndef SMDB_UTILS_APPLICATION_H
#define SMDB_UTILS_APPLICATION_H

#include "smdb/utils/types.h"
#include <memory>
#include <string>
#include <functional>
#include <atomic>

namespace smdb {

/**
 * @brief Application configuration
 */
struct AppConfig {
    // Application info
    std::string app_name = "SMDB";
    std::string app_version = "0.1.0";
    std::string app_description = "Simple Memory Database";

    // Command line arguments
    int argc = 0;
    char** argv = nullptr;

    // Configuration
    std::string config_file;
    std::string log_level = "info";
    std::string log_file;
    bool daemon = false;
    bool verbose = false;

    // Database configuration
    DatabaseConfig db_config;

    // Server configuration (for server mode)
    bool server_mode = false;
    std::string host = "0.0.0.0";
    uint16_t port = 9527;
    size_t max_connections = 100;
    size_t thread_pool_size = 4;
};

/**
 * @brief Application lifecycle states
 */
enum class AppState : uint8_t {
    Created = 0,
    Initializing = 1,
    Initialized = 2,
    Starting = 3,
    Running = 4,
    Stopping = 5,
    Stopped = 6,
    Error = 7
};

/**
 * @brief Application interface
 *
 * Provides a modern, dependency-free application framework
 * that replaces proprietary Application frameworks
 */
class IApplication {
public:
    virtual ~IApplication() = default;

    /**
     * @brief Run the application
     * @param config Application configuration
     * @return Return code (0 for success)
     */
    virtual int run(const AppConfig& config) = 0;

    /**
     * @brief Stop the application
     */
    virtual void stop() = 0;

    /**
     * @brief Get current state
     * @return Application state
     */
    virtual AppState getState() const = 0;

    /**
     * @brief Check if application is running
     * @return true if running
     */
    virtual bool isRunning() const = 0;
};

/**
 * @brief Base application class
 *
 * Implements common application functionality with lifecycle hooks
 */
class ApplicationBase : public IApplication {
public:
    ApplicationBase() : state_(AppState::Created), stop_flag_(false) {}
    virtual ~ApplicationBase() = default;

    /**
     * @brief Main entry point
     */
    int run(const AppConfig& config) final;

    /**
     * @brief Stop the application
     */
    void stop() final {
        stop_flag_.store(true, std::memory_order_release);
    }

    /**
     * @brief Get current state
     */
    AppState getState() const override {
        return state_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check if running
     */
    bool isRunning() const override {
        return getState() == AppState::Running;
    }

protected:
    // ===== Lifecycle hooks to be overridden by subclasses =====

    /**
     * @brief Initialize application (override this)
     * @return Result<void> Success or error
     */
    virtual Result<void> onInitialize() {
        return {};
    }

    /**
     * @brief Called before main loop (override this)
     * @return Result<void> Success or error
     */
    virtual Result<void> onBeforeLoop() {
        return {};
    }

    /**
     * @brief Main loop body (override this)
     * @return Result<void> Success or error
     */
    virtual Result<void> onLoop() {
        return {};
    }

    /**
     * @brief Called after main loop (override this)
     * @return Result<void> Success or error
     */
    virtual Result<void> onAfterLoop() {
        return {};
    }

    /**
     * @brief Cleanup application (override this)
     * @return Result<void> Success or error
     */
    virtual Result<void> onCleanup() {
        return {};
    }

    /**
     * @brief Handle signals (override this)
     */
    virtual void onSignal(int signal) {
        stop();
    }

    /**
     * @brief Get configuration
     */
    const AppConfig& getConfig() const {
        return config_;
    }

    /**
     * @brief Check if should stop
     */
    bool shouldStop() const {
        return stop_flag_.load(std::memory_order_acquire);
    }

private:
    void setState(AppState state) {
        state_.store(state, std::memory_order_release);
    }

    void setupSignals();
    Result<void> initializeLogger();
    Result<void> loadConfigFile();

    AppConfig config_;
    std::atomic<AppState> state_;
    std::atomic<bool> stop_flag_;
};

/**
 * @brief Server application base class
 *
 * For applications that run as a server/daemon
 */
class ServerApplication : public ApplicationBase {
protected:
    Result<void> onBeforeLoop() override;
    Result<void> onAfterLoop() override;

    /**
     * @brief Start server (override this)
     */
    virtual Result<void> onStartServer() = 0;

    /**
     * @brief Stop server (override this)
     */
    virtual Result<void> onStopServer() = 0;

    /**
     * @brief Get server port
     */
    uint16_t getPort() const;

    /**
     * @brief Get max connections
     */
    size_t getMaxConnections() const;
};

/**
 * @brief Console application base class
 *
 * For interactive console applications
 */
class ConsoleApplication : public ApplicationBase {
protected:
    Result<void> onBeforeLoop() override;

    /**
     * @brief Display welcome message (override this)
     */
    virtual void showWelcome() {
        // Default: do nothing
    }

    /**
     * @brief Display prompt (override this)
     */
    virtual std::string getPrompt() const {
        return "smdb";
    }

    /**
     * @brief Process command (override this)
     * @param command Command string
     * @return true to continue, false to exit
     */
    virtual bool processCommand(const std::string& command) = 0;
};

/**
 * @brief Tool application base class
 *
 * For one-shot command-line tools (backup, restore, etc.)
 */
class ToolApplication : public ApplicationBase {
protected:
    Result<void> onBeforeLoop() override;

    /**
     * @brief Execute tool (override this)
     * @return Return code
     */
    virtual int executeTool() = 0;
};

} // namespace smdb

#endif // SMDB_UTILS_APPLICATION_H
