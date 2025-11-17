#include "app.hpp"

#include "analysis/code_analyzer.hpp"
#include "analysis/relationship_engine.hpp"
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

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
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
    std::error_code ec;
    std::filesystem::path initial = std::filesystem::absolute(root_path, ec);
    if (ec) {
        current_path_ = root_path;
    } else {
        current_path_ = initial.lexically_normal().string();
    }

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
    command_executor_ = std::make_unique<CommandExecutor>();

    const auto& theme = theme_manager_->current_theme();
    grid_view_->set_theme(theme);
    list_view_->set_theme(theme);
    top_bar_->set_title("GridFire v2.0");
    top_bar_->set_path(current_path_);

    favorites_manager_->add_favorite({"Quick Access", current_path_});
    if (const char* home = std::getenv("HOME")) {
        favorites_manager_->add_favorite({"Home", home});
    }

    refresh_entries(true);

    log_event(__func__, "initialized");
    return true;
}

void GridFireApp::render() {
    if (!fs_engine_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (refresh_requested_ && now - last_refresh_ >= refresh_interval_) {
        refresh_entries(true);
    }

#ifdef IMGUI_HAS_DOCK
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

    top_bar_->set_path(current_path_);
    TopBar::Interaction top_interaction = top_bar_->render(view_mode_ == ViewMode::Grid, refresh_requested_);
    if (top_interaction.requested_path) {
        change_directory(*top_interaction.requested_path);
    }
    if (top_interaction.refresh_requested) {
        refresh_requested_ = true;
    }
    if (top_interaction.toggle_view_mode) {
        view_mode_ = view_mode_ == ViewMode::Grid ? ViewMode::List : ViewMode::Grid;
    }

    ImGui::Begin("Navigation");
    auto favorites_result = favorites_dropdown_->render(current_path_);
    if (favorites_result.toggle_current) {
        favorites_manager_->toggle_favorite(current_path_);
    }
    if (favorites_result.selected_path) {
        change_directory(*favorites_result.selected_path);
    }
    if (!status_message_.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", status_message_.c_str());
    }
    ImGui::End();

    ImGui::Begin("Workspace");
    if (view_mode_ == ViewMode::Grid) {
        auto interaction = grid_view_->render(selected_index_);
        if (interaction.selected_index) {
            select_entry(*interaction.selected_index);
        }
        if (interaction.activated_index) {
            open_entry(*interaction.activated_index);
        }
    } else {
        auto interaction = list_view_->render(selected_index_);
        if (interaction.selected_index) {
            select_entry(*interaction.selected_index);
        }
        if (interaction.activated_index) {
            open_entry(*interaction.activated_index);
        }
    }
    ImGui::End();

    detail_panel_->set_selection(selected_entry_);
    detail_panel_->render();

    auto command_interaction = command_panel_->render(last_command_exit_code_, last_command_message_);
    if (command_interaction.submitted_command) {
        run_command(*command_interaction.submitted_command);
    }
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

void GridFireApp::refresh_entries(bool force) {
    if (!fs_engine_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (!force && now - last_refresh_ < refresh_interval_) {
        return;
    }

    auto entries = fs_engine_->scan_directory(current_path_);
    std::sort(entries.begin(), entries.end(), [](const FileEntry& lhs, const FileEntry& rhs) {
        if (lhs.is_directory != rhs.is_directory) {
            return lhs.is_directory && !rhs.is_directory;
        }
        return lhs.name < rhs.name;
    });

    current_entries_ = entries;
    grid_view_->set_entries(current_entries_);
    list_view_->set_entries(current_entries_);
    last_refresh_ = now;
    refresh_requested_ = false;

    if (selected_entry_) {
        auto it = std::find_if(current_entries_.begin(), current_entries_.end(), [&](const FileEntry& entry) {
            return entry.full_path == selected_entry_->full_path;
        });
        if (it != current_entries_.end()) {
            selected_index_ = static_cast<size_t>(std::distance(current_entries_.begin(), it));
            selected_entry_ = *it;
        } else {
            selected_index_.reset();
            selected_entry_.reset();
        }
    }

    status_message_ = "Loaded " + std::to_string(current_entries_.size()) + " entries";
}

void GridFireApp::select_entry(size_t index) {
    if (index >= current_entries_.size()) {
        return;
    }
    selected_index_ = index;
    selected_entry_ = current_entries_[index];
}

void GridFireApp::open_entry(size_t index) {
    if (index >= current_entries_.size()) {
        return;
    }
    const auto& entry = current_entries_[index];
    if (entry.is_directory) {
        change_directory(entry.full_path);
    } else {
        status_message_ = "Selected file: " + entry.full_path;
    }
}

void GridFireApp::change_directory(const std::string& path) {
    std::filesystem::path requested(path);
    std::error_code ec;
    if (!std::filesystem::exists(requested, ec)) {
        status_message_ = "Path does not exist";
        return;
    }
    if (!std::filesystem::is_directory(requested, ec)) {
        status_message_ = "Not a directory";
        return;
    }
    std::filesystem::path canonical_path = std::filesystem::canonical(requested, ec);
    if (!ec) {
        current_path_ = canonical_path.string();
    } else {
        current_path_ = std::filesystem::absolute(requested).lexically_normal().string();
    }
    refresh_requested_ = true;
    top_bar_->set_path(current_path_);
    log_event(__func__, "Changed directory to " + current_path_);
}

void GridFireApp::run_command(const std::string& command) {
    if (!command_executor_) {
        return;
    }
    int exit_code = command_executor_->run(command);
    command_panel_->set_last_command(command);
    last_command_exit_code_ = exit_code;
    last_command_message_ = exit_code == 0 ? "Command executed" : "Command failed";
    status_message_ = last_command_message_ + ": " + command;
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
