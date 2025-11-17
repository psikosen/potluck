#include "app.hpp"

#include "analysis/code_analyzer.hpp"
#include "analysis/relationship_engine.hpp"
#include "assets/image_manager.hpp"
#include "command/command_executor.hpp"
#include "database/sqlite_manager.hpp"
#include "favorites/favorites_manager.hpp"
#include "filesystem/filesystem_engine.hpp"
#include "filesystem/stats_engine.hpp"
#include "git/git_manager.hpp"
#include "ui/command_panel.hpp"
#include "ui/detail_panel.hpp"
#include "ui/favorites_dropdown.hpp"
#include "ui/grid_view.hpp"
#include "ui/list_view.hpp"
#include "ui/theme.hpp"
#include "ui/top_bar.hpp"

#include <ctime>
#include <iostream>
#include <optional>
#include <sstream>

namespace {
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", std::gmtime(&now));
    return buffer;
}
}

GridFireApp::GridFireApp() = default;
GridFireApp::~GridFireApp() = default;

bool GridFireApp::initialize(const std::string& root_path) {
    current_path_ = root_path;
    db_ = std::make_unique<SQLiteManager>("gridfire.db");
    if (!db_->initialize()) {
        log_event(__func__, "database initialization failed");
        return false;
    }

    fs_engine_ = std::make_unique<FileSystemEngine>(db_.get());
    stats_engine_ = std::make_unique<DirectoryStatsEngine>();
    code_analyzer_ = std::make_unique<CodeAnalyzer>();
    relationship_engine_ = std::make_unique<RelationshipEngine>();
    git_manager_ = std::make_unique<GitManager>();
    favorites_manager_ = std::make_unique<FavoritesManager>();

    grid_view_ = std::make_unique<GridView>();
    list_view_ = std::make_unique<ListView>();
    detail_panel_ = std::make_unique<DetailPanel>();
    top_bar_ = std::make_unique<TopBar>();
    favorites_dropdown_ = std::make_unique<FavoritesDropdown>(favorites_manager_.get());
    command_panel_ = std::make_unique<CommandPanel>();
    theme_manager_ = std::make_unique<ThemeManager>();

    auto entries = fs_engine_->scan_directory(current_path_);
    grid_view_->set_entries(entries);
    list_view_->set_entries(entries);

    log_event(__func__, "initialized");
    return true;
}

void GridFireApp::render() {
    if (!fs_engine_) {
        return;
    }

    auto entries = fs_engine_->scan_directory(current_path_);
    grid_view_->set_entries(entries);
    list_view_->set_entries(entries);
}

void GridFireApp::shutdown() {
    log_event(__func__, "shutdown begin");
    favorites_dropdown_.reset();
    detail_panel_.reset();
    grid_view_.reset();
    list_view_.reset();
    top_bar_.reset();
    command_panel_.reset();
    theme_manager_.reset();

    favorites_manager_.reset();
    git_manager_.reset();
    relationship_engine_.reset();
    code_analyzer_.reset();
    stats_engine_.reset();
    fs_engine_.reset();
    db_.reset();
    log_event(__func__, "shutdown complete");
}

void GridFireApp::log_event(const std::string& function, const std::string& message) const {
    std::ostringstream oss;
    oss << '{'
        << "\"filename\":\"app.cpp\",";
    oss << "\"timestamp\":\"" << timestamp() << "\",";
    oss << "\"classname\":\"GridFireApp\",";
    oss << "\"function\":\"" << function << "\",";
    oss << "\"system_section\":\"app\",";
    oss << "\"line_num\":0,";
    oss << "\"error\":\"\",";
    oss << "\"db_phase\":\"none\",";
    oss << "\"method\":\"NONE\",";
    oss << "\"message\":\"" << message << "\"";
    oss << '}';
    std::cout << oss.str() << std::endl;
    std::cout << "Continuous skepticism" << std::endl;
}
