#include "command/command_executor.hpp"

#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>

namespace {
std::string shell_escape_single_quotes(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    return escaped;
}
}

CommandResult CommandExecutor::run(const std::string& command, const std::string& working_directory) const {
    CommandResult result;

    std::string escaped_dir = shell_escape_single_quotes(working_directory);
    std::string full_command;
    if (!working_directory.empty()) {
        full_command = "cd '" + escaped_dir + "' && " + command + " 2>&1";
    } else {
        full_command = command + " 2>&1";
    }

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        result.exit_code = -1;
        result.output = "Failed to execute command";
        return result;
    }

    char buffer[256];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.output.append(buffer);
    }

    int status = pclose(pipe);
    if (status == -1) {
        result.exit_code = -1;
    } else if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = status;
    }

    return result;
}
