#pragma once

#include <string>

class CommandPanel {
public:
    CommandPanel() = default;
    void set_last_command(const std::string& command);
    const std::string& last_command() const noexcept { return last_command_; }

private:
    std::string last_command_;
};
