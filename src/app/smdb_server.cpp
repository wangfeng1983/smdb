#include "smdb/app/smdb_server.h"
#include "smdb/utils/logger.h"
#include "smdb/utils/cli.h"
#include <iostream>
#include <memory>

namespace smdb {

//==============================================================================
// SmdbServerApp implementation
//==============================================================================

Result<void> SmdbServerApp::onInitialize() {
    SMDB_LOG_INFO("Initializing SMDB Server...");

    // Load configuration
    auto config_result = loadConfiguration();
    if (!config_result) {
        return std::unexpected("Failed to load configuration: " +
                              config_result.error());
    }

    // Create database instance
    auto db_result = DatabaseFactory::create(getConfig().db_config);
    if (!db_result) {
        return std::unexpected("Failed to create database: " +
                              db_result.error());
    }

    database_ = std::move(db_result.value());

    // Initialize database
    auto init_result = database_->initialize();
    if (!init_result) {
        return std::unexpected("Failed to initialize database: " +
                              init_result.error());
    }

    // Start database
    auto start_result = database_->start();
    if (!start_result) {
        return std::unexpected("Failed to start database: " +
                              start_result.error());
    }

    SMDB_LOG_INFO("Database initialized and started successfully");

    // Setup server-specific configuration
    host_ = getConfig().host;
    port_ = getConfig().port;
    max_connections_ = getConfig().max_connections;

    return {};
}

Result<void> SmdbServerApp::loadConfiguration() {
    // Configuration is already loaded by ApplicationBase
    // Here we can add server-specific config validation

    SMDB_LOG_INFO("Configuration loaded:");
    SMDB_LOG_INFO("  Database: " + getConfig().db_config.name);
    SMDB_LOG_INFO("  Memory pool: " +
                 std::to_string(getConfig().db_config.memory_pool_size / 1024 / 1024) + " MB");
    SMDB_LOG_INFO("  Data dir: " + getConfig().db_config.data_dir);

    return {};
}

Result<void> SmdbServerApp::onStartServer() {
    SMDB_LOG_INFO("Starting server on " + host_ + ":" + std::to_string(port_));

    // TODO: Setup socket, bind, listen
    // This is a placeholder - actual implementation will use socket APIs

    server_running_ = true;

    SMDB_LOG_INFO("Server started successfully");
    SMDB_LOG_INFO("Ready to accept connections");

    return {};
}

Result<void> SmdbServerApp::onLoop() {
    if (!server_running_) {
        return {}; // Server not running, exit loop
    }

    // TODO: Accept connections
    // For now, just sleep to prevent busy-waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return {};
}

Result<void> SmdbServerApp::onStopServer() {
    SMDB_LOG_INFO("Stopping server...");

    server_running_ = false;

    // TODO: Close server socket
    if (server_socket_ != -1) {
        // close(server_socket_);
        server_socket_ = -1;
    }

    SMDB_LOG_INFO("Server stopped");
    return {};
}

Result<void> SmdbServerApp::onCleanup() {
    SMDB_LOG_INFO("Cleaning up...");

    if (database_) {
        // Stop database
        auto stop_result = database_->stop();
        if (!stop_result) {
            SMDB_LOG_WARNING("Failed to stop database: " + stop_result.error());
        }

        // Shutdown database
        auto shutdown_result = database_->shutdown();
        if (!shutdown_result) {
            SMDB_LOG_WARNING("Failed to shutdown database: " + shutdown_result.error());
        }

        database_.reset();
    }

    SMDB_LOG_INFO("Cleanup completed");
    return {};
}

void SmdbServerApp::onSignal(int signal) {
    SMDB_LOG_INFO("Received signal: " + std::to_string(signal));

    // Call base class to set stop flag
    ServerApplication::onSignal(signal);
}

//==============================================================================
// Main entry point
//==============================================================================

int runSmdbServer(int argc, char* argv[]) {
    // Create command line parser
    CommandLineParser cli("smdb-server", "0.1.0",
                         "SMDB Server - Simple Memory Database Server");

    // Add command line options
    cli.addFlag("help", "-h", "--help", "Show this help message");
    cli.addFlag("version", "-V", "--version", "Show version information");
    cli.addFlag("daemon", "-d", "--daemon", "Run as daemon");
    cli.addFlag("verbose", "-v", "--verbose", "Verbose output");

    cli.addOption("config", "-c", "--config", "Configuration file (JSON)");
    cli.addOption("port", "-p", "--port", "Server port", "9527");
    cli.addOption("host", "-H", "--host", "Server host", "0.0.0.0");
    cli.addOption("data-dir", "-D", "--data-dir", "Data directory", "./data");
    cli.addOption("log-file", "-l", "--log-file", "Log file path");
    cli.addOption("log-level", "-L", "--log-level", "Log level (trace,debug,info,warn,error)", "info");

    cli.addOption("memory", "-m", "--memory", "Memory pool size (MB)", "1024");
    cli.addOption("max-connections", "-M", "--max-connections",
                 "Maximum connections", "100");

    // Parse command line
    auto parse_result = cli.parse(argc, argv);
    if (!parse_result) {
        std::cerr << "Error: " << parse_result.error() << std::endl;
        cli.printHelp();
        return 1;
    }

    // Build application config
    AppConfig config;
    config.app_name = "SMDB Server";
    config.app_version = "0.1.0";
    config.app_description = "Simple Memory Database Server";
    config.argc = argc;
    config.argv = argv;

    // Get command line options
    config.daemon = cli.getBool("daemon", false);
    config.verbose = cli.getBool("verbose", false);
    config.config_file = cli.getString("config", "");

    // Database configuration
    config.db_config.name = "smdb";
    config.db_config.port = static_cast<uint16_t>(cli.getInt("port", 9527));
    config.db_config.host = cli.getString("host", "0.0.0.0");
    config.db_config.data_dir = cli.getString("data-dir", "./data");
    config.db_config.max_connections = static_cast<size_t>(
        cli.getInt("max-connections", 100));
    config.db_config.memory_pool_size = static_cast<size_t>(
        cli.getInt("memory", 1024) * 1024 * 1024);

    // Logging
    config.log_file = cli.getString("log-file", "");
    config.log_level = cli.getString("log-level", "info");

    // Server mode
    config.server_mode = true;

    // Create and run application
    SmdbServerApp app;
    return app.run(config);
}

} // namespace smdb

//==============================================================================
// Main function
//==============================================================================

int main(int argc, char* argv[]) {
    return smdb::runSmdbServer(argc, argv);
}
