#include "ui/grid_view.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "imgui.h"
#include "ui/file_icon.hpp"

void GridView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}

GridView::Interaction GridView::render(std::optional<size_t> selected_index) const {
    Interaction interaction;
    context_popup_id_ = "grid_item_context_menu";
    bg_context_popup_id_ = "grid_bg_context_menu";

    if (entries_.empty()) {
        ImGui::TextUnformatted("Directory is empty");

        // Handle background context menu for empty directory
        if (context_menu_ && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(bg_context_popup_id_.c_str());
        }
        if (context_menu_) {
            interaction.context_action = context_menu_->render_background(current_path_, bg_context_popup_id_);
        }
        return interaction;
    }

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float available_width = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>((available_width + spacing) / (tile_width_ + spacing)));
    if (columns <= 0) {
        columns = 1;
    }

    // Track which item (if any) was right-clicked
    static size_t right_clicked_index = SIZE_MAX;
    static std::string drag_payload_path;
    static bool open_item_context_menu = false;

    ImGui::BeginChild("grid-view", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    int column_index = 0;
    bool item_hovered = false;
    bool any_right_clicked = false;

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

        // Check for right-click BEFORE drag source (record it, open popup outside loop)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            right_clicked_index = i;
            open_item_context_menu = true;
            any_right_clicked = true;
            interaction.context_menu_opened = true;
            interaction.context_menu_index = i;
        }

        // Drag source - must be immediately after the item
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            drag_payload_path = entry.full_path;
            ImGui::SetDragDropPayload("FILE_PATH", drag_payload_path.c_str(), drag_payload_path.size() + 1);

            // Drag preview
            const auto& icon_info = FileIconManager::get_icon(entry.name, entry.is_directory);
            ImGui::PushStyleColor(ImGuiCol_Text, icon_info.color);
            ImGui::TextUnformatted(icon_info.icon);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.name.c_str());

            interaction.drag_started = true;
            interaction.drag_index = i;
            ImGui::EndDragDropSource();
        }

        // Drop target (for directories)
        if (entry.is_directory && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
                interaction.drop_received = true;
                interaction.dropped_path = std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                // The target path for the drop
                interaction.context_action.target_path = entry.full_path;
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered()) {
            item_hovered = true;
        }

        // Draw tile content
        auto* draw_list = ImGui::GetWindowDrawList();
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        // Draw file icon
        const auto& icon_info = FileIconManager::get_icon(entry.name, entry.is_directory);
        ImGui::PushStyleColor(ImGuiCol_Text, icon_info.color);
        draw_list->AddText(ImVec2(min.x + 8.0f, min.y + 8.0f),
                           ImGui::GetColorU32(icon_info.color),
                           icon_info.icon);
        ImGui::PopStyleColor();

        // Draw file name (next to icon)
        float icon_width = 24.0f;
        ImU32 text_color = ImGui::GetColorU32(is_selected ? theme_.selection_text_color : theme_.text_color);

        // Truncate name if too long
        std::string display_name = entry.name;
        float max_name_width = tile_width_ - 16.0f - icon_width;
        ImVec2 name_size = ImGui::CalcTextSize(display_name.c_str());
        if (name_size.x > max_name_width) {
            while (display_name.size() > 3 && ImGui::CalcTextSize((display_name + "...").c_str()).x > max_name_width) {
                display_name.pop_back();
            }
            display_name += "...";
        }

        draw_list->AddText(ImVec2(min.x + 8.0f + icon_width, min.y + 8.0f),
                           text_color,
                           display_name.c_str());

        // Draw detail line
        std::string detail_line = entry.is_directory ? "Directory" : size_for_display(entry.size);
        draw_list->AddText(ImVec2(min.x + 8.0f, max.y - 28.0f),
                           ImGui::GetColorU32(theme_.subtle_text_color),
                           detail_line.c_str());

        // Draw modified time
        std::string modified_line = modified_for_display(entry.modified_time);
        draw_list->AddText(ImVec2(min.x + 8.0f, max.y - 14.0f),
                           ImGui::GetColorU32(theme_.subtle_text_color),
                           modified_line.c_str());

        // Tooltip
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

    // Add invisible button to fill remaining space for background drop target
    ImVec2 remaining = ImGui::GetContentRegionAvail();
    if (remaining.x > 0 && remaining.y > 0) {
        ImGui::InvisibleButton("##bg_drop_area", remaining);
        // Check for drop on background (empty space)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
                interaction.drop_received = true;
                interaction.dropped_path = std::string(static_cast<const char*>(payload->Data), payload->DataSize - 1);
                // Leave target_path empty to use current directory
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::EndChild();

    // Open and render context menu OUTSIDE the child window and item loop (no ID scope issues)
    if (open_item_context_menu) {
        ImGui::OpenPopup(context_popup_id_.c_str());
        open_item_context_menu = false;
    }

    // Render context menu for selected item
    if (context_menu_ && right_clicked_index < entries_.size()) {
        auto result = context_menu_->render(entries_[right_clicked_index], context_popup_id_);
        if (result.action != ContextMenuAction::None) {
            interaction.context_action = result;
            right_clicked_index = SIZE_MAX;
        }
    }

    // Background right-click (on empty space)
    if (!item_hovered && !any_right_clicked && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup(bg_context_popup_id_.c_str());
    }
    if (context_menu_) {
        auto bg_result = context_menu_->render_background(current_path_, bg_context_popup_id_);
        if (bg_result.action != ContextMenuAction::None) {
            interaction.context_action = bg_result;
        }
    }

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

void GridView::render_file_icon(const FileEntry& entry) const {
    const auto& icon_info = FileIconManager::get_icon(entry.name, entry.is_directory);
    ImGui::PushStyleColor(ImGuiCol_Text, icon_info.color);
    ImGui::TextUnformatted(icon_info.icon);
    ImGui::PopStyleColor();
}
