#pragma once

#include <optional>
#include <string>
#include <vector>

#include "filesystem/filesystem_engine.hpp"
#include "ui/context_menu.hpp"
#include "ui/theme.hpp"

class GridView {
public:
    struct Interaction {
        std::optional<size_t> selected_index;
        std::optional<size_t> activated_index;
        // Context menu
        bool context_menu_opened = false;
        std::optional<size_t> context_menu_index;
        ContextMenuResult context_action;
        // Drag and drop
        bool drag_started = false;
        std::optional<size_t> drag_index;
        bool drop_received = false;
        std::string dropped_path;
    };

    GridView() = default;

    void set_entries(const std::vector<FileEntry>& entries);
    void set_theme(const ThemeManager::Theme& theme) { theme_ = theme; }
    void set_current_path(const std::string& path) { current_path_ = path; }
    void set_context_menu(ContextMenu* menu) { context_menu_ = menu; }

    Interaction render(std::optional<size_t> selected_index) const;

private:
    ImVec4 color_for_heat(float heat) const;
    std::string size_for_display(uint64_t size) const;
    std::string modified_for_display(std::time_t time) const;
    void render_file_icon(const FileEntry& entry) const;

    std::vector<FileEntry> entries_;
    ThemeManager::Theme theme_;
    std::string current_path_;
    ContextMenu* context_menu_ = nullptr;
    float tile_width_ = 180.0f;
    float tile_height_ = 120.0f;
    mutable std::string context_popup_id_;
    mutable std::string bg_context_popup_id_;
};
