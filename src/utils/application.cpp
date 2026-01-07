#include "smdb/utils/application.h"
#include "smdb/utils/logger.h"
#include "smdb/utils/config.h"
#include "smdb/utils/cli.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

namespace smdb {

//==============================================================================
// ApplicationBase implementation
//==============================================================================

void ApplicationBase::setupSignals() {
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, [this](int) {
        SMDB_LOG_INFO("Received SIGINT, shutting down...");
        onSignal(SIGINT);
    });

    std::signal(SIGTERM, [this](int) {
        SMDB_LOG_INFO("Received SIGTERM, shutting down...");
        onSignal(SIGTERM);
    });

#ifdef SIGQUIT
    std::signal(SIGQUIT, [this](int) {
        SMDB_LOG_INFO("Received SIGQUIT, shutting down...");
        onSignal(SIGQUIT);
    });
#endif

    // Ignore SIGPIPE (should be handled in code)
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif
}

Result<void> ApplicationBase::initializeLogger() {
    auto& logger = SimpleLogger::getInstance();

    // Set log level
    LogLevel level = LogLevel::Info;
    if (config_.verbose) {
        level = LogLevel::Debug;
    }

    std::string level_str = config_.log_level;
    if (level_str == "trace") level = LogLevel::Trace;
    else if (level_str == "debug") level = LogLevel::Debug;
    else if (level_str == "info") level = LogLevel::Info;
    else if (level_str == "warning") level = LogLevel::Warning;
    else if (level_str == "error") level = LogLevel::Error;
    else if (level_str == "critical") level = LogLevel::Critical;

    logger.setLevel(level);

    // Set log file if specified
    if (!config_.log_file.empty()) {
        logger.setLogFile(config_.log_file);
    }

    SMDB_LOG_INFO("Starting " + config_.app_name + " v" + config_.app_version);

    return {};
}

Result<void> ApplicationBase::loadConfigFile() {
    if (config_.config_file.empty()) {
        return {}; // No config file specified
    }

    ConfigManager config_mgr;
    auto result = config_mgr.loadFromFile(config_.config_file);
    if (!result) {
        return Result<void>("Failed to load config file: " + result.error());
    }

    // Override database config from file if present
    // (This is simplified - in production, you'd merge configs properly)

    return {};
}

int ApplicationBase::run(const AppConfig& config) {
    config_ = config;

    try {
        // ===== Phase 1: Initialization =====
        setState(AppState::Initializing);
        setupSignals();
        initializeLogger();
        loadConfigFile();

        auto init_result = onInitialize();
        if (!init_result) {
            SMDB_LOG_CRITICAL("Initialization failed: " + init_result.error());
            setState(AppState::Error);
            return 1;
        }

        setState(AppState::Initialized);

        // ===== Phase 2: Before Loop =====
        setState(AppState::Starting);
        auto before_result = onBeforeLoop();
        if (!before_result) {
            SMDB_LOG_ERROR("Before loop failed: " + before_result.error());
            onCleanup();
            setState(AppState::Error);
            return 1;
        }

        setState(AppState::Running);

        // ===== Phase 3: Main Loop =====
        while (!shouldStop()) {
            auto loop_result = onLoop();
            if (!loop_result) {
                SMDB_LOG_ERROR("Loop error: " + loop_result.error());
                break;
            }

            // Small sleep to prevent busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // ===== Phase 4: After Loop =====
        setState(AppState::Stopping);
        auto after_result = onAfterLoop();
        if (!after_result) {
            SMDB_LOG_WARNING("After loop failed: " + after_result.error());
        }

        // ===== Phase 5: Cleanup =====
        auto cleanup_result = onCleanup();
        if (!cleanup_result) {
            SMDB_LOG_WARNING("Cleanup failed: " + cleanup_result.error());
        }

        setState(AppState::Stopped);
        SMDB_LOG_INFO("Application stopped successfully");

        return 0;

    } catch (const std::exception& e) {
        SMDB_LOG_CRITICAL("Unhandled exception: " + std::string(e.what()));
        setState(AppState::Error);
        return 1;
    } catch (...) {
        SMDB_LOG_CRITICAL("Unknown exception occurred");
        setState(AppState::Error);
        return 1;
    }
}

//==============================================================================
// ServerApplication implementation
//==============================================================================

Result<void> ServerApplication::onBeforeLoop() {
    SMDB_LOG_INFO("Starting server mode...");
    SMDB_LOG_INFO("Server will listen on " + getConfig().host + ":" +
                  std::to_string(getPort()));
    SMDB_LOG_INFO("Max connections: " + std::to_string(getMaxConnections()));

    return onStartServer();
}

Result<void> ServerApplication::onAfterLoop() {
    SMDB_LOG_INFO("Stopping server...");
    return onStopServer();
}

uint16_t ServerApplication::getPort() const {
    return getConfig().port;
}

size_t ServerApplication::getMaxConnections() const {
    return getConfig().max_connections;
}

//==============================================================================
// ConsoleApplication implementation
//==============================================================================

Result<void> ConsoleApplication::onBeforeLoop() {
    showWelcome();

    // For console apps, we don't want a loop - just interactive mode
    // The base loop will handle this
    return {};
}

//==============================================================================
// ToolApplication implementation
//==============================================================================

Result<void> ToolApplication::onBeforeLoop() {
    // Tool applications don't have a main loop
    // Execute once and exit
    int ret = executeTool();
    stop();
    if (ret != 0) {
        return Result<void>("Tool returned error code: " + std::to_string(ret));
    }
    return {};
}

} // namespace smdb
