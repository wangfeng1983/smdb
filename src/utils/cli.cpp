#include "smdb/utils/cli.h"
#include "smdb/utils/logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace smdb {

//==============================================================================
// CommandLineParser implementation
//==============================================================================

void CommandLineParser::addArgument(const CliArg& arg) {
    arguments_.push_back(arg);
}

void CommandLineParser::addFlag(const std::string& name,
                                const std::string& short_name,
                                const std::string& long_name,
                                const std::string& description) {
    CliArg arg(name, short_name, long_name, description, false, true);
    addArgument(arg);
}

void CommandLineParser::addOption(const std::string& name,
                                 const std::string& short_name,
                                 const std::string& long_name,
                                 const std::string& description,
                                 const std::string& default_value) {
    CliArg arg(name, short_name, long_name, description, false, false, default_value);
    addArgument(arg);
}

Result<void> CommandLineParser::parse(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Check for help
        if (arg == "-h" || arg == "--help") {
            printHelp();
            std::exit(0);
        }

        // Check for version
        if (arg == "-V" || arg == "--version") {
            printVersion();
            std::exit(0);
        }

        // Check if it's an option
        if (arg.startswith("--")) {
            // Long option
            auto arg_info = findArg(arg);
            if (!arg_info) {
                return std::unexpected("Unknown option: " + arg);
            }

            if (arg_info->flag) {
                flags_[arg_info->name] = true;
            } else {
                // Option requires a value
                if (i + 1 >= argc) {
                    return std::unexpected("Option " + arg + " requires a value");
                }
                values_[arg_info->name] = argv[++i];
            }

        } else if (arg.startswith("-") && !arg.startswith("--")) {
            // Short option
            auto arg_info = findArg(arg);
            if (!arg_info) {
                return std::unexpected("Unknown option: " + arg);
            }

            if (arg_info->flag) {
                flags_[arg_info->name] = true;
            } else {
                // Option requires a value
                if (i + 1 >= argc) {
                    return std::unexpected("Option " + arg + " requires a value");
                }
                values_[arg_info->name] = argv[++i];
            }

        } else {
            // Positional argument
            positional_args_.push_back(arg);
        }
    }

    // Check required arguments
    for (const auto& arg : arguments_) {
        if (arg.required && !has(arg.name)) {
            return std::unexpected("Required option not specified: " +
                                  arg.long_name);
        }
    }

    return {};
}

bool CommandLineParser::has(const std::string& name) const {
    return values_.find(name) != values_.end() ||
           flags_.find(name) != flags_.end();
}

std::string CommandLineParser::getString(const std::string& name,
                                        const std::string& default_value) const {
    auto it = values_.find(name);
    if (it != values_.end()) {
        return it->second;
    }

    // Check in arguments for default
    const CliArg* arg = findArg(name);
    if (arg && !arg->default_value.empty()) {
        return arg->default_value;
    }

    return default_value;
}

int64_t CommandLineParser::getInt(const std::string& name,
                                  int64_t default_value) const {
    auto str = getString(name, "");
    if (str.empty()) {
        return default_value;
    }
    return std::atoll(str.c_str());
}

bool CommandLineParser::getBool(const std::string& name,
                                bool default_value) const {
    auto it = flags_.find(name);
    if (it != flags_.end()) {
        return it->second;
    }

    auto str = getString(name, "");
    if (str == "true" || str == "1" || str == "yes") {
        return true;
    } else if (str == "false" || str == "0" || str == "no") {
        return false;
    }

    return default_value;
}

std::vector<std::string> CommandLineParser::getPositionalArgs() const {
    return positional_args_;
}

void CommandLineParser::printHelp() const {
    std::cout << app_name_ << " - " << app_description_ << "\n\n";
    std::cout << "Usage: " << app_name_ << " [OPTIONS]\n\n";
    std::cout << "Options:\n";

    for (const auto& arg : arguments_) {
        std::string line = "  ";

        if (!arg.short_name.empty()) {
            line += arg.short_name;
            if (!arg.flag) {
                line += " " + arg.value_name;
            }
            line += ", ";
        }

        if (!arg.long_name.empty()) {
            line += arg.long_name;
            if (!arg.flag) {
                line += "=" + arg.value_name;
            }
        }

        // Pad to column 30
        while (line.length() < 30) {
            line += " ";
        }

        line += arg.description;

        if (!arg.default_value.empty()) {
            line += " (default: " + arg.default_value + ")";
        }

        if (arg.required) {
            line += " [required]";
        }

        std::cout << line << "\n";
    }

    std::cout << "\n  -h, --help     Show this help message\n";
    std::cout << "  -V, --version  Show version information\n";
}

void CommandLineParser::printVersion() const {
    std::cout << app_name_ << " " << app_version_ << "\n";
}

const CliArg* CommandLineParser::findArg(const std::string& name) const {
    // Search by name, short_name, or long_name
    for (const auto& arg : arguments_) {
        if (arg.name == name || arg.short_name == name || arg.long_name == name) {
            return &arg;
        }
    }
    return nullptr;
}

//==============================================================================
// CliApp implementation
//==============================================================================

void CliApp::addCommand(const std::string& name,
                       CommandHandler handler,
                       const std::string& description) {
    commands_[name] = {handler, description};
}

int CliApp::run(int argc, char* argv[]) {
    // Parse global options first
    auto parse_result = parser_.parse(argc, argv);
    if (!parse_result) {
        SMDB_LOG_ERROR(parse_result.error());
        parser_.printHelp();
        return 1;
    }

    auto positional = parser_.getPositionalArgs();

    // If no subcommand, show help
    if (positional.empty()) {
        parser_.printHelp();
        if (!commands_.empty()) {
            std::cout << "\nCommands:\n";
            for (const auto& [name, info] : commands_) {
                std::cout << "  " << name << " - " << info.second << "\n";
            }
        }
        return 0;
    }

    // Check if first positional arg is a command
    std::string cmd = positional[0];
    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        // Remove command name from args
        std::vector<std::string> cmd_args(positional.begin() + 1, positional.end());
        return it->second.first(cmd_args);
    }

    // Unknown command
    SMDB_LOG_ERROR("Unknown command: " + cmd);
    return 1;
}

} // namespace smdb
