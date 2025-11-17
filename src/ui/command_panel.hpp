#pragma once

#include <optional>
#include <string>
#include <vector>

class CommandPanel {
public:
    struct Interaction {
        std::optional<std::string> submitted_command;
    };

    CommandPanel();

    void set_last_command(const std::string& command);
    const std::string& last_command() const noexcept { return last_command_; }
    Interaction render(int last_exit_code, const std::string& last_message);
    void push_history(const std::string& command);

private:
    std::string last_command_;
    std::vector<std::string> history_;
    int history_index_ = -1;
    std::string input_buffer_;
};
