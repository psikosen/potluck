#include "ui/detail_panel.hpp"

#include <ctime>

#include "imgui.h"

void DetailPanel::set_selection(const std::optional<FileEntry>& entry) {
    selection_ = entry;
}

void DetailPanel::render() const {
    ImGui::Begin("Details");
    if (!selection_) {
        ImGui::TextUnformatted("Select a file to see metadata");
        ImGui::End();
        return;
    }

    const FileEntry& entry = *selection_;
    ImGui::Text("Name: %s", entry.name.c_str());
    ImGui::Text("Path: %s", entry.full_path.c_str());
    ImGui::Text("Type: %s", entry.is_directory ? "Directory" : "File");
    ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(entry.size));
    ImGui::Text("Owner: %s", entry.owner.c_str());
    ImGui::Text("Group: %s", entry.group.c_str());
    ImGui::Text("Heat Score: %.2f", entry.heat_score);
    ImGui::Text("Access Count: %d", entry.access_count);

    auto render_time = [](const char* label, std::time_t value) {
        if (value == 0) {
            ImGui::Text("%s: Unknown", label);
            return;
        }
        std::tm tm_buf;
        localtime_r(&value, &tm_buf);
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%c", &tm_buf);
        ImGui::Text("%s: %s", label, buffer);
    };

    render_time("Modified", entry.modified_time);
    render_time("Accessed", entry.accessed_time);
    ImGui::End();
}
