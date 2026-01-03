#ifndef SMDB_UTILS_LOGGER_H
#define SMDB_UTILS_LOGGER_H

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <source_location>

namespace smdb {

/**
 * @brief Log levels
 */
enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
    Off = 6
};

/**
 * @brief Convert log level to string
 */
constexpr const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "FATAL";
        case LogLevel::Off: return "OFF  ";
        default: return "UNKN ";
    }
}

/**
 * @brief Logger interface
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level,
                     const std::string& message,
                     const std::source_location& location = std::source_location::current()) = 0;

    virtual void setLevel(LogLevel level) = 0;
    virtual LogLevel getLevel() const = 0;

    virtual void flush() = 0;
};

/**
 * @brief Simple thread-safe logger implementation
 *
 * This is a lightweight logger that doesn't depend on external libraries.
 * For production use, consider integrating spdlog or similar.
 */
class SimpleLogger : public ILogger {
public:
    static SimpleLogger& getInstance() {
        static SimpleLogger instance;
        return instance;
    }

    void log(LogLevel level,
             const std::string& message,
             const std::source_location& location = std::source_location::current()) override {
        if (level < min_level_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;

        // Timestamp
        oss << '[' << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << ']';

        // Log level
        oss << '[' << logLevelToString(level) << ']';

        // File and line (optional, for debug builds)
        #ifndef NDEBUG
        oss << '[' << location.file_name() << ':' << location.line() << ']';
        #endif

        // Message
        oss << ' ' << message;

        // Output to console
        if (level >= LogLevel::Error) {
            std::cerr << oss.str() << std::endl;
        } else {
            std::cout << oss.str() << std::endl;
        }

        // Output to file if configured
        if (log_file_.is_open()) {
            log_file_ << oss.str() << std::endl;
        }
    }

    void setLevel(LogLevel level) override {
        min_level_ = level;
    }

    LogLevel getLevel() const override {
        return min_level_;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_.close();
        }
        log_file_.open(filename, std::ios::out | std::ios::app);
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout.flush();
        std::cerr.flush();
        if (log_file_.is_open()) {
            log_file_.flush();
        }
    }

private:
    SimpleLogger() : min_level_(LogLevel::Info) {}

    LogLevel min_level_;
    std::mutex mutex_;
    std::ofstream log_file_;
};

/**
 * @brief Convenience macros for logging
 */
#define SMDB_LOG_TRACE(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Trace, msg)

#define SMDB_LOG_DEBUG(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Debug, msg)

#define SMDB_LOG_INFO(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Info, msg)

#define SMDB_LOG_WARNING(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Warning, msg)

#define SMDB_LOG_ERROR(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Error, msg)

#define SMDB_LOG_CRITICAL(msg) \
    smdb::SimpleLogger::getInstance().log(smdb::LogLevel::Critical, msg)

/**
 * @brief Scoped log level changer
 */
class ScopedLogLevel {
public:
    ScopedLogLevel(LogLevel level) {
        auto& logger = SimpleLogger::getInstance();
        old_level_ = logger.getLevel();
        logger.setLevel(level);
    }

    ~ScopedLogLevel() {
        SimpleLogger::getInstance().setLevel(old_level_);
    }

private:
    LogLevel old_level_;
};

} // namespace smdb

#endif // SMDB_UTILS_LOGGER_H
