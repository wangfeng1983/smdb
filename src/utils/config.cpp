#include "smdb/utils/config.h"
#include "smdb/utils/logger.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cctype>

namespace smdb {

//==============================================================================
// ConfigManager implementation
//==============================================================================

Result<void> ConfigManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return std::unexpected("Failed to open config file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Parse JSON (simplified)
    auto parser_result = SimpleJsonParser::parse(buffer.str());
    if (!parser_result) {
        return std::unexpected("Failed to parse config file: " + parser_result.error());
    }

    *this = parser_result.value();
    return {};
}

void ConfigManager::loadFromEnv(const std::string& prefix) {
    // Load environment variables
    extern char** environ;

    for (char** env = environ; *env != nullptr; ++env) {
        std::string env_str = *env;
        size_t eq_pos = env_str.find('=');

        if (eq_pos != std::string::npos) {
            std::string key = env_str.substr(0, eq_pos);
            std::string value = env_str.substr(eq_pos + 1);

            // Check if key starts with prefix
            if (key.startswith(prefix)) {
                // Convert "SMDB_DB_NAME" to "db.name"
                std::string config_key = key.substr(prefix.length());
                for (size_t i = 0; i < config_key.length(); ++i) {
                    if (config_key[i] == '_') {
                        config_key[i] = '.';
                    } else {
                        config_key[i] = std::tolower(config_key[i]);
                    }
                }

                setString(config_key, value);
                SMDB_LOG_DEBUG("Loaded from env: " + config_key + " = " + value);
            }
        }
    }
}

std::string ConfigManager::getString(const std::string& key,
                                     const std::string& default_value) {
    auto it = values_.find(key);
    if (it != values_.end() && std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return default_value;
}

int64_t ConfigManager::getInt(const std::string& key, int64_t default_value) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        if (std::holds_alternative<int64_t>(it->second)) {
            return std::get<int64_t>(it->second);
        } else if (std::holds_alternative<std::string>(it->second)) {
            return std::atoll(std::get<std::string>(it->second).c_str());
        }
    }
    return default_value;
}

bool ConfigManager::getBool(const std::string& key, bool default_value) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        if (std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second);
        } else if (std::holds_alternative<std::string>(it->second)) {
            std::string val = std::get<std::string>(it->second);
            return (val == "true" || val == "1" || val == "yes");
        }
    }
    return default_value;
}

double ConfigManager::getDouble(const std::string& key, double default_value) {
    auto it = values_.find(key);
    if (it != values_.end()) {
        if (std::holds_alternative<double>(it->second)) {
            return std::get<double>(it->second);
        } else if (std::holds_alternative<int64_t>(it->second)) {
            return static_cast<double>(std::get<int64_t>(it->second));
        } else if (std::holds_alternative<std::string>(it->second)) {
            return std::atof(std::get<std::string>(it->second).c_str());
        }
    }
    return default_value;
}

void ConfigManager::set(const std::string& key, const ConfigValue& value) {
    values_[key] = value;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    values_[key] = value;
}

void ConfigManager::setInt(const std::string& key, int64_t value) {
    values_[key] = value;
}

void ConfigManager::setBool(const std::string& key, bool value) {
    values_[key] = value;
}

void ConfigManager::setDouble(const std::string& key, double value) {
    values_[key] = value;
}

bool ConfigManager::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}

std::vector<std::string> ConfigManager::getKeysWithPrefix(const std::string& prefix) const {
    std::vector<std::string> keys;
    for (const auto& [key, value] : values_) {
        if (key.starts_with(prefix)) {
            keys.push_back(key);
        }
    }
    return keys;
}

void ConfigManager::clear() {
    values_.clear();
}

DatabaseConfig ConfigManager::toDatabaseConfig() const {
    DatabaseConfig config;

    config.name = getString("db.name", "smdb");
    config.memory_pool_size = static_cast<size_t>(
        getInt("db.memory_pool_size", 1024 * 1024 * 1024));
    config.data_dir = getString("db.data_dir", "./data");
    config.enable_persistence = getBool("db.enable_persistence", true);
    config.enable_recovery = getBool("db.enable_recovery", true);
    config.checkpoint_interval_sec = static_cast<size_t>(
        getInt("db.checkpoint_interval", 60));
    config.max_connections = static_cast<size_t>(
        getInt("db.max_connections", 100));

    return config;
}

//==============================================================================
// SimpleJsonParser implementation
//==============================================================================

Result<ConfigManager> SimpleJsonParser::parse(const std::string& content) {
    ConfigManager config;

    size_t pos = 0;

    // Skip leading whitespace
    pos = skipWhitespace(content, pos);

    // Expect '{'
    if (pos >= content.length() || content[pos] != '{') {
        return std::unexpected("Expected '{' at start of JSON");
    }
    pos++;

    while (pos < content.length()) {
        pos = skipWhitespace(content, pos);

        if (content[pos] == '}') {
            // End of object
            break;
        }

        // Parse key-value pair
        auto pair_result = parsePair(content, pos);
        if (!pair_result) {
            return std::unexpected(pair_result.error());
        }

        auto [key, value] = pair_result.value();
        config.set(key, value);

        pos = skipWhitespace(content, pos);

        // Expect ',' or '}'
        if (content[pos] == ',') {
            pos++;
        } else if (content[pos] == '}') {
            break;
        } else {
            return std::unexpected("Expected ',' or '}' after pair");
        }
    }

    return config;
}

size_t SimpleJsonParser::skipWhitespace(const std::string& content, size_t pos) {
    while (pos < content.length() && std::isspace(content[pos])) {
        pos++;
    }
    return pos;
}

Result<std::pair<std::string, ConfigValue>>
SimpleJsonParser::parsePair(const std::string& content, size_t pos) {
    pos = skipWhitespace(content, pos);

    // Expect string key
    if (pos >= content.length() || content[pos] != '"') {
        return std::unexpected("Expected string key");
    }

    std::string key = parseString(content, pos);
    pos += key.length() + 2; // Skip quotes

    pos = skipWhitespace(content, pos);

    // Expect ':'
    if (pos >= content.length() || content[pos] != ':') {
        return std::unexpected("Expected ':' after key");
    }
    pos++;

    pos = skipWhitespace(content, pos);

    // Parse value
    auto value_result = parseValue(content, pos);
    if (!value_result) {
        return std::unexpected(value_result.error());
    }

    return {{key, value_result.value()}};
}

Result<ConfigValue> SimpleJsonParser::parseValue(const std::string& content, size_t pos) {
    pos = skipWhitespace(content, pos);

    if (pos >= content.length()) {
        return std::unexpected("Unexpected end of input");
    }

    char c = content[pos];

    if (c == '"') {
        // String
        std::string str = parseString(content, pos);
        return str;
    } else if (c == 't' || c == 'f') {
        // Boolean
        std::string bool_str;
        while (pos < content.length() &&
               (content[pos] == 't' || content[pos] == 'r' ||
                content[pos] == 'u' || content[pos] == 'e' ||
                content[pos] == 'f' || content[pos] == 'a' ||
                content[pos] == 'l' || content[pos] == 's')) {
            bool_str += content[pos++];
        }
        return (bool_str == "true");
    } else if (c == '-' || std::isdigit(c)) {
        // Number
        std::string num_str;
        while (pos < content.length() &&
               (std::isdigit(content[pos]) || content[pos] == '.' ||
                content[pos] == '-' || content[pos] == 'e' ||
                content[pos] == 'E')) {
            num_str += content[pos++];
        }

        if (num_str.find('.') != std::string::npos) {
            return std::atof(num_str.c_str());
        } else {
            return std::atoll(num_str.c_str());
        }
    } else if (c == '{' || c == '[') {
        // Object or array (not supported in this simple parser)
        return std::unexpected("Nested objects/arrays not supported");
    }

    return std::unexpected("Unexpected character in value");
}

std::string SimpleJsonParser::parseString(const std::string& content, size_t pos) {
    // Skip opening quote
    pos++;

    std::string result;
    while (pos < content.length() && content[pos] != '"') {
        if (content[pos] == '\\' && pos + 1 < content.length()) {
            // Escape sequence
            pos++;
            char next = content[pos];
            switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += next; break;
            }
            pos++;
        } else {
            result += content[pos++];
        }
    }

    return result;
}

} // namespace smdb
