#include "ui/list_view.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

#include "imgui.h"

void ListView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}

ListView::Interaction ListView::render(std::optional<size_t> selected_index) const {
    Interaction interaction;

    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX;

    if (ImGui::BeginTable("file-list", 4, table_flags, ImVec2(0, 0))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < entries_.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i));

            bool is_selected = selected_index && *selected_index == i;
            std::string label = entries_[i].name + "##row";
            if (entries_[i].name.empty()) {
                label = entries_[i].full_path + "##row";
            }

            if (ImGui::Selectable(label.c_str(),
                                   is_selected,
                                   ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                interaction.selected_index = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    interaction.activated_index = i;
                }
            }

            ImGui::TableNextColumn();
            if (entries_[i].is_directory) {
                ImGui::TextUnformatted("<DIR>");
            } else {
                ImGui::TextUnformatted(size_for_display(entries_[i].size).c_str());
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(time_for_display(entries_[i].modified_time).c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s:%s", entries_[i].owner.c_str(), entries_[i].group.c_str());

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(entries_[i].full_path.c_str());
                ImGui::Text("Heat: %.2f", entries_[i].heat_score);
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    return interaction;
}

std::string ListView::size_for_display(uint64_t size) const {
    if (size == 0) {
        return "0 B";
    }
    static constexpr const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(size);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(value < 10.0 ? 1 : 0) << value << ' ' << kUnits[unit];
    return oss.str();
}

std::string ListView::time_for_display(std::time_t time) const {
    if (time == 0) {
        return "Unknown";
    }
    std::tm tm_buf;
    localtime_r(&time, &tm_buf);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm_buf);
    return buffer;
}
