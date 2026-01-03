#ifndef SMDB_UTILS_CLI_H
#define SMDB_UTILS_CLI_H

#include "smdb/utils/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace smdb {

/**
 * @brief Command line argument
 */
struct CliArg {
    std::string name;           // Argument name
    std::string short_name;     // Short name (e.g., "-h")
    std::string long_name;      // Long name (e.g., "--help")
    std::string description;    // Help description
    bool required = false;      // Is required
    bool flag = false;          // Is boolean flag
    std::string default_value;  // Default value
    std::string value_name;     // Value name in help (e.g., "FILE")

    CliArg() = default;

    CliArg(const std::string& n, const std::string& s, const std::string& l,
           const std::string& desc, bool req = false, bool is_flag = false,
           const std::string& def = "", const std::string& vn = "VALUE")
        : name(n), short_name(s), long_name(l), description(desc),
          required(req), flag(is_flag), default_value(def), value_name(vn) {}
};

/**
 * @brief Command line parser
 *
 * Simple argument parser without external dependencies.
 * For production, consider using CLI11 or similar.
 */
class CommandLineParser {
public:
    CommandLineParser(const std::string& app_name,
                     const std::string& app_version,
                     const std::string& app_description)
        : app_name_(app_name), app_version_(app_version),
          app_description_(app_description) {}

    /**
     * @brief Add an argument
     */
    void addArgument(const CliArg& arg);

    /**
     * @brief Add a flag (boolean option)
     */
    void addFlag(const std::string& name,
                const std::string& short_name,
                const std::string& long_name,
                const std::string& description);

    /**
     * @brief Add a required option
     */
    void addOption(const std::string& name,
                  const std::string& short_name,
                  const std::string& long_name,
                  const std::string& description,
                  const std::string& default_value = "");

    /**
     * @brief Parse command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Result<void> Success or error message
     */
    Result<void> parse(int argc, char* argv[]);

    /**
     * @brief Check if a flag was set
     * @param name Argument name
     * @return true if flag was set
     */
    bool has(const std::string& name) const;

    /**
     * @brief Get string value
     * @param name Argument name
     * @param default_value Default value
     * @return String value
     */
    std::string getString(const std::string& name,
                         const std::string& default_value = "") const;

    /**
     * @brief Get integer value
     * @param name Argument name
     * @param default_value Default value
     * @return Integer value
     */
    int64_t getInt(const std::string& name, int64_t default_value = 0) const;

    /**
     * @brief Get boolean value
     * @param name Argument name
     * @param default_value Default value
     * @return Boolean value
     */
    bool getBool(const std::string& name, bool default_value = false) const;

    /**
     * @brief Get position arguments
     * @return Vector of positional arguments
     */
    std::vector<std::string> getPositionalArgs() const;

    /**
     * @brief Print help message
     */
    void printHelp() const;

    /**
     * @brief Print version
     */
    void printVersion() const;

private:
    std::string app_name_;
    std::string app_version_;
    std::string app_description_;

    std::vector<CliArg> arguments_;
    std::unordered_map<std::string, std::string> values_;
    std::unordered_map<std::string, bool> flags_;
    std::vector<std::string> positional_args_;

    const CliArg* findArg(const std::string& name) const;
};

/**
 * @brief Command line interface helper
 *
 * Provides a simple way to build CLI applications
 */
class CliApp {
public:
    using CommandHandler = std::function<int(const std::vector<std::string>&)>;

    CliApp(const std::string& name,
           const std::string& version,
           const std::string& description)
        : parser_(name, version, description) {}

    /**
     * @brief Add a subcommand
     * @param name Command name
     * @param handler Command handler
     * @param description Command description
     */
    void addCommand(const std::string& name,
                   CommandHandler handler,
                   const std::string& description);

    /**
     * @brief Parse and run
     * @param argc Argument count
     * @param argv Argument values
     * @return Return code
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Get parser
     */
    CommandLineParser& parser() { return parser_; }

private:
    CommandLineParser parser_;
    std::unordered_map<std::string, std::pair<CommandHandler, std::string>> commands_;
};

} // namespace smdb

#endif // SMDB_UTILS_CLI_H
