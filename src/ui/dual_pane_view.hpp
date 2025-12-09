#pragma once

#include <optional>

#include "ui/context_menu.hpp"
#include "ui/pane_state.hpp"

class DualPaneView {
public:
    enum class SplitOrientation { Vertical, Horizontal };

    struct PaneRenderResult {
        std::optional<size_t> selected_index;
        std::optional<size_t> activated_index;
        bool tab_path_changed = false;
        std::string new_path;
        // Context menu
        ContextMenuResult context_action;
        // Drag and drop
        bool drop_received = false;
        std::string dropped_path;
        std::string drop_target_path;  // Directory where item was dropped
    };

    struct Interaction {
        PaneRenderResult left;
        PaneRenderResult right;
        bool switch_active = false;
        PaneIdentifier requested_active = PaneIdentifier::Left;
        bool copy_active_to_other = false;
        bool copy_other_to_active = false;
        bool move_active_to_other = false;
        bool move_other_to_active = false;
        bool sync_active_to_other = false;
        bool swap_requested = false;
        bool orientation_changed = false;
        SplitOrientation new_orientation = SplitOrientation::Vertical;
    };

    DualPaneView();

    Interaction render(PaneState& left, PaneState& right, PaneIdentifier active_pane);
    SplitOrientation orientation() const noexcept { return orientation_; }
    void set_orientation(SplitOrientation orientation);

private:
    PaneRenderResult render_pane(PaneState& pane, bool is_active) const;
    void render_action_column(Interaction& interaction, PaneIdentifier active_pane);
    void update_divider();
    
    // Draggable splitter rendering
    bool render_vertical_splitter(const char* id, float height, float& ratio, float total_width);
    bool render_horizontal_splitter(const char* id, float width, float& ratio, float total_height);

    SplitOrientation orientation_ = SplitOrientation::Vertical;
    float divider_ratio_ = 0.5f;
};
