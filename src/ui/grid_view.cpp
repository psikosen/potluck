#include "ui/grid_view.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "imgui.h"

void GridView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}

GridView::Interaction GridView::render(std::optional<size_t> selected_index) const {
    Interaction interaction;
    if (entries_.empty()) {
        ImGui::TextUnformatted("Directory is empty");
        return interaction;
    }

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float available_width = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>((available_width + spacing) / (tile_width_ + spacing)));
    if (columns <= 0) {
        columns = 1;
    }

    ImGui::BeginChild("grid-view", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    int column_index = 0;
    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        if (column_index > 0) {
            ImGui::SameLine(0.0f, spacing);
        }

        ImGui::PushID(static_cast<int>(i));

        ImVec4 heat_color = color_for_heat(entry.heat_score);
        ImVec4 hover_color = ImVec4(heat_color.x + 0.1f, heat_color.y + 0.1f, heat_color.z + 0.1f, heat_color.w);
        bool is_selected = selected_index && *selected_index == i;

        if (is_selected) {
            heat_color = theme_.selection_color;
            hover_color = theme_.selection_color;
        }

        ImGui::PushStyleColor(ImGuiCol_Header, heat_color);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hover_color);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, hover_color);

        bool clicked = ImGui::Selectable("##tile",
                                         is_selected,
                                         ImGuiSelectableFlags_AllowDoubleClick,
                                         ImVec2(tile_width_, tile_height_));

        auto* draw_list = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        draw_list->AddText(ImVec2(min.x + 8.0f, min.y + 8.0f),
                           ImGui::GetColorU32(is_selected ? theme_.selection_text_color : theme_.text_color),
                           entry.name.c_str());

        std::string detail_line = entry.is_directory ? "Directory" : size_for_display(entry.size);
        draw_list->AddText(ImVec2(min.x + 8.0f, max.y - 28.0f),
                           ImGui::GetColorU32(theme_.subtle_text_color),
                           detail_line.c_str());

        std::string modified_line = modified_for_display(entry.modified_time);
        draw_list->AddText(ImVec2(min.x + 8.0f, max.y - 14.0f),
                           ImGui::GetColorU32(theme_.subtle_text_color),
                           modified_line.c_str());

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(entry.full_path.c_str());
            ImGui::Text("Heat: %.2f  Accesses: %d", entry.heat_score, entry.access_count);
            ImGui::EndTooltip();
        }

        if (clicked) {
            interaction.selected_index = i;
        }
        if (clicked && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            interaction.activated_index = i;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopID();

        column_index = (column_index + 1) % columns;
        if (column_index == 0) {
            ImGui::NewLine();
        }
    }
    ImGui::EndChild();
    return interaction;
}

ImVec4 GridView::color_for_heat(float heat) const {
    float clamped = std::clamp(heat, 0.0f, 1.0f);
    float inv = 1.0f - clamped;
    return ImVec4(theme_.heat_hot_color.x * clamped + theme_.heat_cold_color.x * inv,
                  theme_.heat_hot_color.y * clamped + theme_.heat_cold_color.y * inv,
                  theme_.heat_hot_color.z * clamped + theme_.heat_cold_color.z * inv,
                  0.35f + 0.35f * clamped);
}

std::string GridView::size_for_display(uint64_t size) const {
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

std::string GridView::modified_for_display(std::time_t time) const {
    if (time == 0) {
        return "Unknown";
    }
    std::tm tm_buf;
    localtime_r(&time, &tm_buf);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm_buf);
    return buffer;
}
