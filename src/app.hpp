#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "filesystem/filesystem_engine.hpp"

class SQLiteManager;
class FileSystemEngine;
class DirectoryStatsEngine;
class CodeAnalyzer;
class RelationshipEngine;
class GitManager;
class FavoritesManager;
class GridView;
class DetailPanel;
class TopBar;
class FavoritesDropdown;
class ListView;
class CommandPanel;
class ThemeManager;
class CommandExecutor;

class GridFireApp {
public:
    GridFireApp();
    ~GridFireApp();

    bool initialize(const std::string& root_path = ".");
    void render();
    void shutdown();

    const std::string& current_path() const noexcept { return current_path_; }

private:
    enum class ViewMode { Grid, List };

    void refresh_entries(bool force = false);
    void select_entry(size_t index);
    void open_entry(size_t index);
    void change_directory(const std::string& path);
    void run_command(const std::string& command);

    void log_event(const std::string& function, const std::string& message) const;

    std::unique_ptr<SQLiteManager> db_;
    std::unique_ptr<FileSystemEngine> fs_engine_;
    std::unique_ptr<DirectoryStatsEngine> stats_engine_;
    std::unique_ptr<CodeAnalyzer> code_analyzer_;
    std::unique_ptr<RelationshipEngine> relationship_engine_;
    std::unique_ptr<GitManager> git_manager_;
    std::unique_ptr<FavoritesManager> favorites_manager_;

    std::unique_ptr<GridView> grid_view_;
    std::unique_ptr<ListView> list_view_;
    std::unique_ptr<DetailPanel> detail_panel_;
    std::unique_ptr<TopBar> top_bar_;
    std::unique_ptr<FavoritesDropdown> favorites_dropdown_;
    std::unique_ptr<CommandPanel> command_panel_;
    std::unique_ptr<ThemeManager> theme_manager_;
    std::unique_ptr<CommandExecutor> command_executor_;

    std::string current_path_;
    std::vector<FileEntry> current_entries_;
    std::optional<size_t> selected_index_;
    std::optional<FileEntry> selected_entry_;
    std::chrono::steady_clock::time_point last_refresh_;
    std::chrono::milliseconds refresh_interval_{std::chrono::seconds(2)};
    bool refresh_requested_ = true;
    ViewMode view_mode_ = ViewMode::Grid;
    std::string status_message_;
    std::string last_command_message_;
    int last_command_exit_code_ = 0;
};
