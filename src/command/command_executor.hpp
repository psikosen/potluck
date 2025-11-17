#pragma once

#include <string>

class CommandExecutor {
public:
    CommandExecutor() = default;
    int run(const std::string& command) const;
};
