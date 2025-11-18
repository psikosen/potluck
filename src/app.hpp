#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "filesystem/filesystem_engine.hpp"
#include "ui/pane_state.hpp"
#include "ui/view_mode.hpp"

class SQLiteManager;
class FileSystemEngine;
class DirectoryStatsEngine;
class CodeAnalyzer;
class RelationshipEngine;
class GitManager;
class FavoritesManager;
class DetailPanel;
class TopBar;
class FavoritesDropdown;
class CommandPanel;
class ThemeManager;
class CommandExecutor;
class DualPaneView;

class GridFireApp {
public:
    GridFireApp();
    ~GridFireApp();

    bool initialize(const std::string& root_path = ".");
    void render();
    void shutdown();

    const std::string& current_path() const noexcept { return active_pane_->current_path; }

private:
    void initialize_pane(PaneState& pane, const std::string& root_path);
    void refresh_pane_entries(PaneState& pane, bool force = false);
    void select_entry(PaneState& pane, size_t index);
    void open_entry(PaneState& pane, size_t index);
    void change_directory(PaneState& pane, const std::string& path);
    void copy_between_panes(PaneState& source, PaneState& destination, bool move);
    void sync_panes(const PaneState& source, PaneState& destination);
    void swap_panes();
    PaneState& pane(PaneIdentifier id);
    PaneState& active_pane();
    PaneState& other_pane();

    void run_command(const std::string& command);

    void log_event(const std::string& function, const std::string& message) const;

    std::unique_ptr<SQLiteManager> db_;
    std::unique_ptr<FileSystemEngine> fs_engine_;
    std::unique_ptr<DirectoryStatsEngine> stats_engine_;
    std::unique_ptr<CodeAnalyzer> code_analyzer_;
    std::unique_ptr<RelationshipEngine> relationship_engine_;
    std::unique_ptr<GitManager> git_manager_;
    std::unique_ptr<FavoritesManager> favorites_manager_;

    std::unique_ptr<DetailPanel> detail_panel_;
    std::unique_ptr<TopBar> top_bar_;
    std::unique_ptr<FavoritesDropdown> favorites_dropdown_;
    std::unique_ptr<CommandPanel> command_panel_;
    std::unique_ptr<ThemeManager> theme_manager_;
    std::unique_ptr<CommandExecutor> command_executor_;
    std::unique_ptr<DualPaneView> dual_pane_view_;

    PaneState left_pane_{PaneIdentifier::Left, "Left Pane"};
    PaneState right_pane_{PaneIdentifier::Right, "Right Pane"};
    PaneState* active_pane_ = &left_pane_;
    std::chrono::milliseconds refresh_interval_{std::chrono::seconds(2)};
    std::string status_message_;
    std::string last_command_message_;
    int last_command_exit_code_ = 0;
};
