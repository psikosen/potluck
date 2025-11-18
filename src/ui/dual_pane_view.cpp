#include "ui/dual_pane_view.hpp"

#include <algorithm>
#include <cfloat>

#include "imgui.h"

DualPaneView::DualPaneView() = default;

void DualPaneView::set_orientation(SplitOrientation orientation) {
    if (orientation_ != orientation) {
        orientation_ = orientation;
    }
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
    const float control_extent = 120.0f;

    if (orientation_ == SplitOrientation::Vertical) {
        float left_width = std::max(available.x - control_extent, 0.0f) * divider_ratio_;
        float right_width = std::max(available.x - control_extent, 0.0f) - left_width;

        ImGui::BeginChild("LeftPane", ImVec2(left_width, 0), true);
        interaction.left = render_pane(left, active_pane == PaneIdentifier::Left);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("PaneActions", ImVec2(control_extent, 0), false);
        render_action_column(interaction, active_pane);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("RightPane", ImVec2(right_width, 0), true);
        interaction.right = render_pane(right, active_pane == PaneIdentifier::Right);
        ImGui::EndChild();
    } else {
        float top_height = std::max(available.y - control_extent, 0.0f) * divider_ratio_;
        float bottom_height = std::max(available.y - control_extent, 0.0f) - top_height;

        ImGui::BeginChild("TopPane", ImVec2(0, top_height), true);
        interaction.left = render_pane(left, active_pane == PaneIdentifier::Left);
        ImGui::EndChild();

        ImGui::BeginChild("PaneActionsHorizontal", ImVec2(0, control_extent), false);
        render_action_column(interaction, active_pane);
        ImGui::EndChild();

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

    auto tab_result = pane.tabs.render(pane.pane_label, is_active);
    if (tab_result.path_changed) {
        result.tab_path_changed = true;
        result.new_path = tab_result.new_path;
    }

    ImGui::TextColored(is_active ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", pane.current_path.c_str());
    ImGui::Separator();

    if (pane.view_mode == ViewMode::Grid) {
        auto grid_interaction = pane.grid_view->render(pane.selected_index);
        if (grid_interaction.selected_index) {
            result.selected_index = grid_interaction.selected_index;
        }
        if (grid_interaction.activated_index) {
            result.activated_index = grid_interaction.activated_index;
        }
    } else {
        auto list_interaction = pane.list_view->render(pane.selected_index);
        if (list_interaction.selected_index) {
            result.selected_index = list_interaction.selected_index;
        }
        if (list_interaction.activated_index) {
            result.activated_index = list_interaction.activated_index;
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
