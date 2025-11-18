#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "filesystem/filesystem_engine.hpp"
#include "ui/grid_view.hpp"
#include "ui/list_view.hpp"
#include "ui/tab_system.hpp"
#include "ui/view_mode.hpp"

enum class PaneIdentifier { Left, Right };

struct PaneState {
    PaneState() = default;
    PaneState(PaneIdentifier identifier, std::string label_text)
        : id(identifier), pane_label(std::move(label_text)) {}

    PaneIdentifier id;
    std::string pane_label;
    std::string current_path;
    std::vector<FileEntry> entries;
    std::optional<size_t> selected_index;
    std::optional<FileEntry> selected_entry;
    bool refresh_requested = true;
    ViewMode view_mode = ViewMode::Grid;
    bool dirty = false;
    std::chrono::steady_clock::time_point last_refresh{};
    std::unique_ptr<GridView> grid_view;
    std::unique_ptr<ListView> list_view;
    TabSystem tabs;
};
