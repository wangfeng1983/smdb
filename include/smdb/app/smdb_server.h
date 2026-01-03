#ifndef SMDB_APP_SMDB_SERVER_H
#define SMDB_APP_SMDB_SERVER_H

#include "smdb/utils/application.h"
#include "smdb/core/database.h"
#include <memory>

namespace smdb {

/**
 * @brief SMDB Server Application
 *
 * This is the main server application that runs as a daemon/service
 * and accepts client connections over the network.
 *
 * Usage:
 *   smdb-server --config /path/to/config.json
 *   smdb-server --port 9527 --data-dir /data
 */
class SmdbServerApp : public ServerApplication {
public:
    SmdbServerApp() = default;
    ~SmdbServerApp() override = default;

protected:
    // ===== Lifecycle hooks =====

    Result<void> onInitialize() override;

    Result<void> onStartServer() override;

    Result<void> onLoop() override;

    Result<void> onStopServer() override;

    Result<void> onCleanup() override;

    void onSignal(int signal) override;

private:
    /**
     * @brief Load server configuration
     */
    Result<void> loadConfiguration();

    /**
     * @brief Setup signal handlers
     */
    void setupSignalHandlers();

    /**
     * @brief Accept client connections
     */
    Result<void> acceptConnections();

    /**
     * @brief Handle client connection
     */
    void handleClient(int socket_fd);

    // Server components
    std::unique_ptr<IDatabase> database_;

    // Server state
    int server_socket_ = -1;
    bool server_running_ = false;

    // Configuration
    std::string host_;
    uint16_t port_;
    size_t max_connections_;
    std::vector<std::string> allowed_ips_;
    size_t idle_timeout_sec_;
};

/**
 * @brief Create and run the SMDB server
 *
 * This is the main entry point for the server application.
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int runSmdbServer(int argc, char* argv[]);

} // namespace smdb

#endif // SMDB_APP_SMDB_SERVER_H
