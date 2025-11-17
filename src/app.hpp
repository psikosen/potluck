#pragma once

#include <memory>
#include <string>

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

class GridFireApp {
public:
    GridFireApp();
    ~GridFireApp();

    bool initialize(const std::string& root_path = ".");
    void render();
    void shutdown();

    const std::string& current_path() const noexcept { return current_path_; }

private:
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

    std::string current_path_;
    bool show_detail_panel_ = true;
};
