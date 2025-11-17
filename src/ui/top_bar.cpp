#include "ui/top_bar.hpp"

#include "imgui.h"
#include "imgui_stdlib.h"

#include <filesystem>

void TopBar::set_title(const std::string& title) {
    title_ = title;
}

void TopBar::set_path(const std::string& path) {
    if (!path_dirty_) {
        path_buffer_ = path;
    }
}

TopBar::Interaction TopBar::render(bool is_grid_mode, bool busy) {
    Interaction interaction;

    if (ImGui::Begin("TopBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::TextUnformatted(title_.c_str());
        ImGui::SameLine();

        ImGui::SetNextItemWidth(400.0f);
        if (ImGui::InputText("##path", &path_buffer_, ImGuiInputTextFlags_EnterReturnsTrue)) {
            interaction.requested_path = path_buffer_;
            path_dirty_ = false;
        } else if (ImGui::IsItemDeactivatedAfterEdit()) {
            interaction.requested_path = path_buffer_;
            path_dirty_ = false;
        } else if (ImGui::IsItemActive()) {
            path_dirty_ = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Go")) {
            interaction.requested_path = path_buffer_;
        }

        ImGui::SameLine();
        if (ImGui::Button(busy ? "Refreshing" : "Refresh")) {
            interaction.refresh_requested = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(is_grid_mode ? "List" : "Grid")) {
            interaction.toggle_view_mode = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Up")) {
            std::filesystem::path current(path_buffer_);
            if (current.has_parent_path()) {
                interaction.requested_path = current.parent_path().string();
            }
        }
    }
    ImGui::End();
    return interaction;
}
