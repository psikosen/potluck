#include "ui/dual_pane_view.hpp"

#include <algorithm>
#include <cfloat>
#include <filesystem>

#include "imgui.h"

DualPaneView::DualPaneView() = default;

void DualPaneView::set_orientation(SplitOrientation orientation) {
    if (orientation_ != orientation) {
        orientation_ = orientation;
    }
}

bool DualPaneView::render_vertical_splitter(const char* id, float height, float& ratio, float total_width) {
    const float splitter_width = 8.0f;
    const float half_splitter = splitter_width * 0.5f;
    
    ImGui::SameLine(0.0f, 0.0f);
    
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 splitter_min = ImVec2(cursor_pos.x - half_splitter, cursor_pos.y);
    ImVec2 splitter_max = ImVec2(cursor_pos.x + half_splitter, cursor_pos.y + height);
    
    ImGui::InvisibleButton(id, ImVec2(splitter_width, height));
    
    bool is_hovered = ImGui::IsItemHovered();
    bool is_active = ImGui::IsItemActive();
    
    // Change cursor to resize cursor when hovering
    if (is_hovered || is_active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    
    // Draw splitter bar
    ImU32 splitter_color;
    if (is_active) {
        splitter_color = ImGui::GetColorU32(ImVec4(0.9f, 0.7f, 0.2f, 1.0f));  // Active: gold
    } else if (is_hovered) {
        splitter_color = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.8f, 1.0f));  // Hover: light blue
    } else {
        splitter_color = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.4f, 1.0f));  // Normal: dark gray
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        ImVec2(splitter_min.x + 2.0f, splitter_min.y + 4.0f),
        ImVec2(splitter_max.x - 2.0f, splitter_max.y - 4.0f),
        splitter_color,
        2.0f
    );
    
    // Handle dragging
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta = ImGui::GetIO().MouseDelta.x;
        if (total_width > 0.0f) {
            ratio += delta / total_width;
            ratio = std::clamp(ratio, 0.15f, 0.85f);
            return true;
        }
    }
    
    ImGui::SameLine(0.0f, 0.0f);
    return false;
}

bool DualPaneView::render_horizontal_splitter(const char* id, float width, float& ratio, float total_height) {
    const float splitter_height = 8.0f;
    const float half_splitter = splitter_height * 0.5f;
    
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 splitter_min = ImVec2(cursor_pos.x, cursor_pos.y - half_splitter);
    ImVec2 splitter_max = ImVec2(cursor_pos.x + width, cursor_pos.y + half_splitter);
    
    ImGui::InvisibleButton(id, ImVec2(width, splitter_height));
    
    bool is_hovered = ImGui::IsItemHovered();
    bool is_active = ImGui::IsItemActive();
    
    // Change cursor to resize cursor when hovering
    if (is_hovered || is_active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    
    // Draw splitter bar
    ImU32 splitter_color;
    if (is_active) {
        splitter_color = ImGui::GetColorU32(ImVec4(0.9f, 0.7f, 0.2f, 1.0f));  // Active: gold
    } else if (is_hovered) {
        splitter_color = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.8f, 1.0f));  // Hover: light blue
    } else {
        splitter_color = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.4f, 1.0f));  // Normal: dark gray
    }
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        ImVec2(splitter_min.x + 4.0f, splitter_min.y + 2.0f),
        ImVec2(splitter_max.x - 4.0f, splitter_max.y - 2.0f),
        splitter_color,
        2.0f
    );
    
    // Handle dragging
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta = ImGui::GetIO().MouseDelta.y;
        if (total_height > 0.0f) {
            ratio += delta / total_height;
            ratio = std::clamp(ratio, 0.15f, 0.85f);
            return true;
        }
    }
    
    return false;
}

DualPaneView::Interaction DualPaneView::render(PaneState& left,
                                               PaneState& right,
                                               PaneIdentifier active_pane) {
    Interaction interaction;
    interaction.new_orientation = orientation_;

    update_divider();
    auto& io = ImGui::GetIO();
    if (!io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        interaction.switch_active = true;
        interaction.requested_active = active_pane == PaneIdentifier::Left ? PaneIdentifier::Right : PaneIdentifier::Left;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_U)) {
        interaction.swap_requested = true;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_E)) {
        orientation_ = orientation_ == SplitOrientation::Vertical ? SplitOrientation::Horizontal : SplitOrientation::Vertical;
        interaction.orientation_changed = true;
        interaction.new_orientation = orientation_;
    }

    ImVec2 available = ImGui::GetContentRegionAvail();
    const float control_extent = 100.0f;
    const float splitter_size = 8.0f;

    if (orientation_ == SplitOrientation::Vertical) {
        // Calculate widths accounting for splitters
        float pane_area = std::max(available.x - control_extent - splitter_size * 2, 0.0f);
        float left_width = pane_area * divider_ratio_;
        float right_width = pane_area - left_width;

        // Left pane
        ImGui::BeginChild("LeftPane", ImVec2(left_width, 0), true);
        interaction.left = render_pane(left, active_pane == PaneIdentifier::Left);
        ImGui::EndChild();

        // Left splitter (between left pane and action column)
        render_vertical_splitter("##left_splitter", available.y, divider_ratio_, pane_area);

        // Action column
        ImGui::BeginChild("PaneActions", ImVec2(control_extent, 0), false);
        render_action_column(interaction, active_pane);
        ImGui::EndChild();

        // Right splitter (between action column and right pane)
        ImGui::SameLine(0.0f, 0.0f);
        render_vertical_splitter("##right_splitter", available.y, divider_ratio_, pane_area);

        // Right pane
        ImGui::BeginChild("RightPane", ImVec2(right_width, 0), true);
        interaction.right = render_pane(right, active_pane == PaneIdentifier::Right);
        ImGui::EndChild();
    } else {
        // Calculate heights accounting for splitters
        float pane_area = std::max(available.y - control_extent - splitter_size * 2, 0.0f);
        float top_height = pane_area * divider_ratio_;
        float bottom_height = pane_area - top_height;

        // Top pane
        ImGui::BeginChild("TopPane", ImVec2(0, top_height), true);
        interaction.left = render_pane(left, active_pane == PaneIdentifier::Left);
        ImGui::EndChild();

        // Top splitter
        render_horizontal_splitter("##top_splitter", available.x, divider_ratio_, pane_area);

        // Action row
        ImGui::BeginChild("PaneActionsHorizontal", ImVec2(0, control_extent), false);
        render_action_column(interaction, active_pane);
        ImGui::EndChild();

        // Bottom splitter
        render_horizontal_splitter("##bottom_splitter", available.x, divider_ratio_, pane_area);

        // Bottom pane
        ImGui::BeginChild("BottomPane", ImVec2(0, bottom_height), true);
        interaction.right = render_pane(right, active_pane == PaneIdentifier::Right);
        ImGui::EndChild();
    }

    return interaction;
}

DualPaneView::PaneRenderResult DualPaneView::render_pane(PaneState& pane, bool is_active) const {
    PaneRenderResult result;
    if (!pane.grid_view || !pane.list_view) {
        return result;
    }

    // Set up context menu and current path for views
    pane.grid_view->set_context_menu(&pane.context_menu);
    pane.grid_view->set_current_path(pane.current_path);
    pane.list_view->set_context_menu(&pane.context_menu);
    pane.list_view->set_current_path(pane.current_path);

    auto tab_result = pane.tabs.render(pane.pane_label, is_active);
    if (tab_result.path_changed) {
        result.tab_path_changed = true;
        result.new_path = tab_result.new_path;
    }

    ImGui::BeginGroup();
    ImVec4 color = is_active ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    std::filesystem::path current_path(pane.current_path);

    auto render_segment_button = [&](const std::string& label, const std::string& target_path, bool first) {
        if (!first) {
            ImGui::SameLine();
            ImGui::TextUnformatted("/");
            ImGui::SameLine();
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        bool clicked = ImGui::SmallButton(label.c_str());
        ImGui::PopStyleColor();
        if (clicked) {
            result.tab_path_changed = true;
            result.new_path = target_path;
        }
    };

    bool first_segment = true;
    std::filesystem::path accumulated;

    if (current_path.is_absolute()) {
        accumulated = std::filesystem::path("/");
        render_segment_button("/", accumulated.string(), first_segment);
        first_segment = false;
        for (const auto& part : current_path.relative_path()) {
            std::string segment = part.string();
            if (segment.empty()) {
                continue;
            }
            accumulated /= part;
            render_segment_button(segment, accumulated.string(), first_segment);
            first_segment = false;
        }
    } else {
        accumulated.clear();
        for (const auto& part : current_path) {
            std::string segment = part.string();
            if (segment.empty()) {
                continue;
            }
            if (accumulated.empty()) {
                accumulated = std::filesystem::path(segment);
            } else {
                accumulated /= part;
            }
            render_segment_button(segment, accumulated.string(), first_segment);
            first_segment = false;
        }
    }

    ImGui::EndGroup();
    ImGui::Separator();

    if (pane.view_mode == ViewMode::Grid) {
        auto grid_interaction = pane.grid_view->render(pane.selected_index);
        if (grid_interaction.selected_index) {
            result.selected_index = grid_interaction.selected_index;
        }
        if (grid_interaction.activated_index) {
            result.activated_index = grid_interaction.activated_index;
        }
        // Context menu action
        if (grid_interaction.context_action.action != ContextMenuAction::None) {
            result.context_action = grid_interaction.context_action;
        }
        // Drag and drop
        if (grid_interaction.drop_received) {
            result.drop_received = true;
            result.dropped_path = grid_interaction.dropped_path;
            result.drop_target_path = grid_interaction.context_action.target_path.empty()
                                        ? pane.current_path
                                        : grid_interaction.context_action.target_path;
        }
    } else {
        auto list_interaction = pane.list_view->render(pane.selected_index);
        if (list_interaction.selected_index) {
            result.selected_index = list_interaction.selected_index;
        }
        if (list_interaction.activated_index) {
            result.activated_index = list_interaction.activated_index;
        }
        // Context menu action
        if (list_interaction.context_action.action != ContextMenuAction::None) {
            result.context_action = list_interaction.context_action;
        }
        // Drag and drop
        if (list_interaction.drop_received) {
            result.drop_received = true;
            result.dropped_path = list_interaction.dropped_path;
            result.drop_target_path = list_interaction.context_action.target_path.empty()
                                        ? pane.current_path
                                        : list_interaction.context_action.target_path;
        }
    }

    return result;
}

void DualPaneView::render_action_column(Interaction& interaction, PaneIdentifier active_pane) {
    bool active_left = active_pane == PaneIdentifier::Left;
    std::string copy_to_other = std::string("Copy ") + (active_left ? u8"→" : u8"←");
    std::string copy_from_other = std::string("Copy ") + (active_left ? u8"←" : u8"→");
    std::string move_to_other = std::string("Move ") + (active_left ? u8"→" : u8"←");
    std::string move_from_other = std::string("Move ") + (active_left ? u8"←" : u8"→");
    std::string sync_label = std::string("Sync ") + (active_left ? u8"→" : u8"←");

    if (ImGui::Button(copy_to_other.c_str(), ImVec2(-FLT_MIN, 0))) {
        interaction.copy_active_to_other = true;
    }
    if (ImGui::Button(copy_from_other.c_str(), ImVec2(-FLT_MIN, 0))) {
        interaction.copy_other_to_active = true;
    }
    if (ImGui::Button(move_to_other.c_str(), ImVec2(-FLT_MIN, 0))) {
        interaction.move_active_to_other = true;
    }
    if (ImGui::Button(move_from_other.c_str(), ImVec2(-FLT_MIN, 0))) {
        interaction.move_other_to_active = true;
    }
    if (ImGui::Button(sync_label.c_str(), ImVec2(-FLT_MIN, 0))) {
        interaction.sync_active_to_other = true;
    }
    if (ImGui::Button("Swap Panes", ImVec2(-FLT_MIN, 0))) {
        interaction.swap_requested = true;
    }
    std::string orientation_label = orientation_ == SplitOrientation::Vertical ? "Horizontal Split" : "Vertical Split";
    if (ImGui::Button(orientation_label.c_str(), ImVec2(-FLT_MIN, 0))) {
        orientation_ = orientation_ == SplitOrientation::Vertical ? SplitOrientation::Horizontal : SplitOrientation::Vertical;
        interaction.orientation_changed = true;
        interaction.new_orientation = orientation_;
    }
    ImGui::SliderFloat("Ratio", &divider_ratio_, 0.2f, 0.8f, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
}

void DualPaneView::update_divider() {
    divider_ratio_ = std::clamp(divider_ratio_, 0.2f, 0.8f);
}
