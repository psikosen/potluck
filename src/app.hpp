#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "filesystem/filesystem_engine.hpp"
#include "ui/context_menu.hpp"
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
    void handle_context_action(PaneState& pane, const ContextMenuResult& action);
    void handle_drop(PaneState& pane, const std::string& source_path, const std::string& target_dir);

    // Context menu action handlers
    void open_in_editor(const std::string& path);
    void open_in_terminal(const std::string& path);
    void delete_entry(PaneState& pane, const std::string& path);
    void rename_entry(PaneState& pane, const std::string& path);
    void create_new_folder(PaneState& pane, const std::string& parent_path);
    void create_new_file(PaneState& pane, const std::string& parent_path);
    void copy_path_to_clipboard(const std::string& path);
    void paste_from_clipboard(PaneState& pane, const std::string& target_dir);

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

    // Dialog state
    bool show_rename_dialog_ = false;
    bool show_delete_dialog_ = false;
    bool show_new_folder_dialog_ = false;
    bool show_new_file_dialog_ = false;
    std::string dialog_target_path_;
    char dialog_input_buffer_[256] = {0};
    PaneState* dialog_pane_ = nullptr;
};
