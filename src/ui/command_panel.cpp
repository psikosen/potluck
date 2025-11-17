#include "ui/command_panel.hpp"

#include "imgui.h"
#include "imgui_stdlib.h"

#include <algorithm>

CommandPanel::CommandPanel() {
    input_buffer_.reserve(256);
}

void CommandPanel::set_last_command(const std::string& command) {
    last_command_ = command;
}

CommandPanel::Interaction CommandPanel::render(int last_exit_code, const std::string& last_message) {
    Interaction interaction;
    ImGui::Begin("Command Panel");

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText("Command", &input_buffer_, flags)) {
        if (!input_buffer_.empty()) {
            interaction.submitted_command = input_buffer_;
            push_history(input_buffer_);
            input_buffer_.clear();
        }
    }

    if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (!history_.empty()) {
            history_index_ = std::clamp(history_index_ + 1, 0, static_cast<int>(history_.size() - 1));
            input_buffer_ = history_[history_.size() - 1 - history_index_];
        }
    } else if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (history_index_ > 0) {
            history_index_--;
            input_buffer_ = history_[history_.size() - 1 - history_index_];
        } else {
            history_index_ = -1;
            input_buffer_.clear();
        }
    }

    if (!last_command_.empty()) {
        ImGui::Separator();
        ImGui::Text("Last: %s", last_command_.c_str());
        ImGui::Text("Exit Code: %d", last_exit_code);
        if (!last_message.empty()) {
            ImGui::TextWrapped("%s", last_message.c_str());
        }
    }

    ImGui::End();
    return interaction;
}

void CommandPanel::push_history(const std::string& command) {
    history_.push_back(command);
    if (history_.size() > 50) {
        history_.erase(history_.begin());
    }
    history_index_ = -1;
}
