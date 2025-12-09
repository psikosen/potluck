#include "ui/list_view.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

#include "imgui.h"
#include "ui/file_icon.hpp"

void ListView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}

ListView::Interaction ListView::render(std::optional<size_t> selected_index) const {
    Interaction interaction;
    context_popup_id_ = "list_item_context_menu";
    bg_context_popup_id_ = "list_bg_context_menu";

    static size_t right_clicked_index = SIZE_MAX;
    static std::string drag_payload_path;
    static bool open_item_context_menu = false;

    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoHostExtendX;

    bool item_hovered = false;
    bool any_right_clicked = false;

    if (ImGui::BeginTable("file-list", 5, table_flags, ImVec2(0, 0))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30.0f);  // Icon column
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            ImGui::TableNextRow();
            ImGui::PushID(static_cast<int>(i));

            bool is_selected = selected_index && *selected_index == i;

            // Icon column
            ImGui::TableNextColumn();
            const auto& icon_info = FileIconManager::get_icon(entry.name, entry.is_directory);
            ImGui::PushStyleColor(ImGuiCol_Text, icon_info.color);
            ImGui::TextUnformatted(icon_info.icon);
            ImGui::PopStyleColor();

            // Name column with selectable
            ImGui::TableNextColumn();
            std::string label = entry.name + "##row";
            if (entry.name.empty()) {
                label = entry.full_path + "##row";
            }

            if (ImGui::Selectable(label.c_str(),
                                   is_selected,
                                   ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                interaction.selected_index = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    interaction.activated_index = i;
                }
            }

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
                    interaction.context_action.target_path = entry.full_path;
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemHovered()) {
                item_hovered = true;
            }

            // Size column
            ImGui::TableNextColumn();
            if (entry.is_directory) {
                ImGui::TextUnformatted("<DIR>");
            } else {
                ImGui::TextUnformatted(size_for_display(entry.size).c_str());
            }

            // Modified column
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(time_for_display(entry.modified_time).c_str());

            // Owner column
            ImGui::TableNextColumn();
            ImGui::Text("%s:%s", entry.owner.c_str(), entry.group.c_str());

            // Tooltip
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(entry.full_path.c_str());
                ImGui::Text("Heat: %.2f", entry.heat_score);
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();

        // Add invisible button to fill remaining space for background drop target
        ImVec2 remaining = ImGui::GetContentRegionAvail();
        if (remaining.x > 0 && remaining.y > 0) {
            ImGui::InvisibleButton("##list_bg_drop_area", remaining);
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
    }

    // Open and render context menu OUTSIDE the table and item loop (no ID scope issues)
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

    // Background right-click (on empty space below table)
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
