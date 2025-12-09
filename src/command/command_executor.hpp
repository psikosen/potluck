#pragma once

#include <string>

struct CommandResult {
    int exit_code = -1;
    std::string output;
};

class CommandExecutor {
public:
    CommandExecutor() = default;
    CommandResult run(const std::string& command, const std::string& working_directory) const;
};
