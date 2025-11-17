#include "command/command_executor.hpp"

#include <cstdlib>

int CommandExecutor::run(const std::string& command) const {
    return std::system(command.c_str());
}
