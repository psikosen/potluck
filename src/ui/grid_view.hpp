#pragma once

#include <optional>
#include <string>
#include <vector>

#include "filesystem/filesystem_engine.hpp"
#include "ui/theme.hpp"

class GridView {
public:
    struct Interaction {
        std::optional<size_t> selected_index;
        std::optional<size_t> activated_index;
    };

    GridView() = default;

    void set_entries(const std::vector<FileEntry>& entries);
    void set_theme(const ThemeManager::Theme& theme) { theme_ = theme; }

    Interaction render(std::optional<size_t> selected_index) const;

private:
    ImVec4 color_for_heat(float heat) const;
    std::string size_for_display(uint64_t size) const;
    std::string modified_for_display(std::time_t time) const;

    std::vector<FileEntry> entries_;
    ThemeManager::Theme theme_;
    float tile_width_ = 180.0f;
    float tile_height_ = 120.0f;
};
