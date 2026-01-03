#ifndef SMDB_UTILS_CONFIG_H
#define SMDB_UTILS_CONFIG_H

#include "smdb/utils/types.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <variant>

namespace smdb {

/**
 * @brief Configuration value type
 */
using ConfigValue = std::variant<
    std::monostate,
    bool,
    int64_t,
    double,
    std::string
>;

/**
 * @brief Simple configuration manager
 *
 * This provides a lightweight configuration system.
 * For production, integrate with nlohmann/json or similar.
 */
class ConfigManager {
public:
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path (JSON format)
     * @return Result<void> Success or error
     */
    Result<void> loadFromFile(const std::string& filename);

    /**
     * @brief Load configuration from environment variables
     * @param prefix Environment variable prefix (e.g., "SMDB_")
     */
    void loadFromEnv(const std::string& prefix = "SMDB_");

    /**
     * @brief Get string value
     * @param key Configuration key
     * @param default_value Default value if not found
     * @return String value
     */
    std::string getString(const std::string& key,
                         const std::string& default_value = "");

    /**
     * @brief Get integer value
     * @param key Configuration key
     * @param default_value Default value if not found
     * @return Integer value
     */
    int64_t getInt(const std::string& key, int64_t default_value = 0);

    /**
     * @brief Get boolean value
     * @param key Configuration key
     * @param default_value Default value if not found
     * @return Boolean value
     */
    bool getBool(const std::string& key, bool default_value = false);

    /**
     * @brief Get double value
     * @param key Configuration key
     * @param default_value Default value if not found
     * @return Double value
     */
    double getDouble(const std::string& key, double default_value = 0.0);

    /**
     * @brief Set a value
     * @param key Configuration key
     * @param value Configuration value
     */
    void set(const std::string& key, const ConfigValue& value);

    /**
     * @brief Set string value
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief Set integer value
     */
    void setInt(const std::string& key, int64_t value);

    /**
     * @brief Set boolean value
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief Set double value
     */
    void setDouble(const std::string& key, double value);

    /**
     * @brief Check if key exists
     * @param key Configuration key
     * @return true if exists
     */
    bool has(const std::string& key) const;

    /**
     * @brief Get all keys with prefix
     * @param prefix Key prefix
     * @return Vector of matching keys
     */
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;

    /**
     * @brief Clear all configuration
     */
    void clear();

    /**
     * @brief Convert to DatabaseConfig
     * @return Database configuration
     */
    DatabaseConfig toDatabaseConfig() const;

private:
    std::unordered_map<std::string, ConfigValue> values_;
};

/**
 * @brief Helper to parse simple JSON-like configuration
 *
 * Note: This is a minimal parser for demonstration.
 * In production, use nlohmann/json or similar.
 */
class SimpleJsonParser {
public:
    static Result<ConfigManager> parse(const std::string& content);

private:
    static size_t skipWhitespace(const std::string& content, size_t pos);
    static Result<std::pair<std::string, ConfigValue>> parsePair(
        const std::string& content, size_t pos);
    static Result<ConfigValue> parseValue(const std::string& content, size_t pos);
    static std::string parseString(const std::string& content, size_t pos);
};

} // namespace smdb

#endif // SMDB_UTILS_CONFIG_H
