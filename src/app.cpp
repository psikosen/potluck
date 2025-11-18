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
#include "ui/dual_pane_view.hpp"
#include "ui/favorites_dropdown.hpp"
#include "ui/theme.hpp"
#include "ui/top_bar.hpp"

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
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

std::string pane_label(PaneIdentifier id) {
    return id == PaneIdentifier::Left ? "left" : "right";
}
}

GridFireApp::GridFireApp() = default;
GridFireApp::~GridFireApp() = default;

bool GridFireApp::initialize(const std::string& root_path) {
    std::error_code ec;
    std::filesystem::path initial = std::filesystem::absolute(root_path, ec);
    std::string resolved_path = ec ? root_path : initial.lexically_normal().string();

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

    detail_panel_ = std::make_unique<DetailPanel>();
    top_bar_ = std::make_unique<TopBar>();
    favorites_dropdown_ = std::make_unique<FavoritesDropdown>(favorites_manager_.get());
    command_panel_ = std::make_unique<CommandPanel>();
    theme_manager_ = std::make_unique<ThemeManager>();
    command_executor_ = std::make_unique<CommandExecutor>();
    dual_pane_view_ = std::make_unique<DualPaneView>();

    top_bar_->set_title("GridFire v2.0");

    favorites_manager_->add_favorite({"Quick Access", resolved_path});
    if (const char* home = std::getenv("HOME")) {
        favorites_manager_->add_favorite({"Home", home});
    }

    initialize_pane(left_pane_, resolved_path);
    initialize_pane(right_pane_, resolved_path);
    active_pane_ = &left_pane_;

    log_event(__func__, "initialized");
    return true;
}

void GridFireApp::initialize_pane(PaneState& pane, const std::string& root_path) {
    pane.grid_view = std::make_unique<GridView>();
    pane.list_view = std::make_unique<ListView>();
    const auto& theme = theme_manager_->current_theme();
    pane.grid_view->set_theme(theme);
    pane.list_view->set_theme(theme);
    pane.current_path = root_path;
    pane.tabs.set_database(db_.get());
    pane.tabs.set_pane_id(pane_label(pane.id));
    pane.tabs.initialize(root_path);
    change_directory(pane, pane.tabs.active_tab().path);
    pane.refresh_requested = true;
}

void GridFireApp::render() {
    if (!fs_engine_) {
        return;
    }

    refresh_pane_entries(left_pane_, left_pane_.refresh_requested);
    refresh_pane_entries(right_pane_, right_pane_.refresh_requested);

#ifdef IMGUI_HAS_DOCK
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

    top_bar_->set_path(active_pane().current_path);
    TopBar::Interaction top_interaction = top_bar_->render(active_pane().view_mode == ViewMode::Grid,
                                                          active_pane().refresh_requested);
    if (top_interaction.requested_path) {
        change_directory(active_pane(), *top_interaction.requested_path);
    }
    if (top_interaction.refresh_requested) {
        active_pane().refresh_requested = true;
    }
    if (top_interaction.toggle_view_mode) {
        active_pane().view_mode = active_pane().view_mode == ViewMode::Grid ? ViewMode::List : ViewMode::Grid;
    }

    ImGui::Begin("Navigation");
    ImGui::SeparatorText("Left Pane");
    ImGui::PushID("left-pane");
    auto left_favorites = favorites_dropdown_->render(left_pane_.current_path);
    ImGui::PopID();
    if (left_favorites.toggle_current) {
        favorites_manager_->toggle_favorite(left_pane_.current_path);
    }
    if (left_favorites.selected_path) {
        change_directory(left_pane_, *left_favorites.selected_path);
    }

    ImGui::Separator();
    ImGui::SeparatorText("Right Pane");
    ImGui::PushID("right-pane");
    auto right_favorites = favorites_dropdown_->render(right_pane_.current_path);
    ImGui::PopID();
    if (right_favorites.toggle_current) {
        favorites_manager_->toggle_favorite(right_pane_.current_path);
    }
    if (right_favorites.selected_path) {
        change_directory(right_pane_, *right_favorites.selected_path);
    }

    if (!status_message_.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", status_message_.c_str());
    }
    ImGui::End();

    ImGui::Begin("Workspace");
    auto pane_interaction = dual_pane_view_->render(left_pane_, right_pane_, active_pane_->id);

    if (pane_interaction.left.selected_index) {
        select_entry(left_pane_, *pane_interaction.left.selected_index);
    }
    if (pane_interaction.left.activated_index) {
        open_entry(left_pane_, *pane_interaction.left.activated_index);
    }
    if (pane_interaction.left.tab_path_changed) {
        change_directory(left_pane_, pane_interaction.left.new_path);
    }

    if (pane_interaction.right.selected_index) {
        select_entry(right_pane_, *pane_interaction.right.selected_index);
    }
    if (pane_interaction.right.activated_index) {
        open_entry(right_pane_, *pane_interaction.right.activated_index);
    }
    if (pane_interaction.right.tab_path_changed) {
        change_directory(right_pane_, pane_interaction.right.new_path);
    }

    if (pane_interaction.switch_active) {
        active_pane_ = &pane(pane_interaction.requested_active);
    }

    if (pane_interaction.copy_active_to_other) {
        copy_between_panes(*active_pane_, other_pane(), false);
    }
    if (pane_interaction.copy_other_to_active) {
        copy_between_panes(other_pane(), *active_pane_, false);
    }
    if (pane_interaction.move_active_to_other) {
        copy_between_panes(*active_pane_, other_pane(), true);
    }
    if (pane_interaction.move_other_to_active) {
        copy_between_panes(other_pane(), *active_pane_, true);
    }
    if (pane_interaction.sync_active_to_other) {
        sync_panes(*active_pane_, other_pane());
    }
    if (pane_interaction.swap_requested) {
        swap_panes();
    }
    ImGui::End();

    detail_panel_->set_selection(active_pane().selected_entry);
    detail_panel_->render();

    auto command_interaction = command_panel_->render(last_command_exit_code_, last_command_message_);
    if (command_interaction.submitted_command) {
        run_command(*command_interaction.submitted_command);
    }

    left_pane_.tabs.persist();
    right_pane_.tabs.persist();
}

void GridFireApp::shutdown() {
    log_event(__func__, "shutdown begin");
    left_pane_.tabs.persist();
    right_pane_.tabs.persist();
    favorites_dropdown_.reset();
    detail_panel_.reset();
    top_bar_.reset();
    command_panel_.reset();
    theme_manager_.reset();
    dual_pane_view_.reset();

    favorites_manager_.reset();
    git_manager_.reset();
    relationship_engine_.reset();
    code_analyzer_.reset();
    stats_engine_.reset();
    fs_engine_.reset();
    db_.reset();
    log_event(__func__, "shutdown complete");
}

void GridFireApp::refresh_pane_entries(PaneState& pane, bool force) {
    if (!fs_engine_) {
        return;
    }
    auto now = std::chrono::steady_clock::now();
    if (!force && pane.last_refresh.time_since_epoch().count() != 0 &&
        now - pane.last_refresh < refresh_interval_) {
        return;
    }

    auto entries = fs_engine_->scan_directory(pane.current_path);
    std::sort(entries.begin(), entries.end(), [](const FileEntry& lhs, const FileEntry& rhs) {
        if (lhs.is_directory != rhs.is_directory) {
            return lhs.is_directory && !rhs.is_directory;
        }
        return lhs.name < rhs.name;
    });

    pane.entries = entries;
    if (pane.grid_view) {
        pane.grid_view->set_entries(pane.entries);
    }
    if (pane.list_view) {
        pane.list_view->set_entries(pane.entries);
    }
    pane.last_refresh = now;
    pane.refresh_requested = false;

    if (pane.selected_entry) {
        auto it = std::find_if(pane.entries.begin(), pane.entries.end(), [&](const FileEntry& entry) {
            return entry.full_path == pane.selected_entry->full_path;
        });
        if (it != pane.entries.end()) {
            pane.selected_index = static_cast<size_t>(std::distance(pane.entries.begin(), it));
            pane.selected_entry = *it;
        } else {
            pane.selected_index.reset();
            pane.selected_entry.reset();
        }
    }

    status_message_ = "Loaded " + std::to_string(pane.entries.size()) + " entries in " + pane.current_path;
}

void GridFireApp::select_entry(PaneState& pane, size_t index) {
    if (index >= pane.entries.size()) {
        return;
    }
    pane.selected_index = index;
    pane.selected_entry = pane.entries[index];
}

void GridFireApp::open_entry(PaneState& pane, size_t index) {
    if (index >= pane.entries.size()) {
        return;
    }
    const auto& entry = pane.entries[index];
    if (entry.is_directory) {
        change_directory(pane, entry.full_path);
    } else {
        status_message_ = "Selected file: " + entry.full_path;
    }
}

void GridFireApp::change_directory(PaneState& pane, const std::string& path) {
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
        pane.current_path = canonical_path.string();
    } else {
        pane.current_path = std::filesystem::absolute(requested).lexically_normal().string();
    }
    pane.refresh_requested = true;
    pane.tabs.active_tab().path = pane.current_path;
    pane.tabs.mark_dirty();
    pane.tabs.persist();
    log_event(__func__, "Changed " + pane_label(pane.id) + " pane to " + pane.current_path);
}

void GridFireApp::copy_between_panes(PaneState& source, PaneState& destination, bool move) {
    if (!source.selected_entry) {
        status_message_ = "Select an entry first";
        return;
    }
    const auto& entry = *source.selected_entry;
    std::filesystem::path target = std::filesystem::path(destination.current_path) / entry.name;
    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        status_message_ = "Target already exists: " + target.string();
        return;
    }

    try {
        if (entry.is_directory) {
            if (move) {
                std::filesystem::rename(entry.full_path, target);
            } else {
                std::filesystem::copy(entry.full_path, target, std::filesystem::copy_options::recursive);
            }
        } else {
            if (move) {
                std::filesystem::rename(entry.full_path, target);
            } else {
                std::filesystem::copy_file(entry.full_path, target);
            }
        }
        status_message_ = (move ? "Moved " : "Copied ") + entry.name + " to " + destination.current_path;
        log_event(__func__, status_message_);
        source.refresh_requested = true;
        destination.refresh_requested = true;
    } catch (const std::filesystem::filesystem_error& error) {
        status_message_ = std::string("Operation failed: ") + error.what();
        log_event(__func__, status_message_);
    }
}

void GridFireApp::sync_panes(const PaneState& source, PaneState& destination) {
    change_directory(destination, source.current_path);
}

void GridFireApp::swap_panes() {
    std::swap(left_pane_.tabs, right_pane_.tabs);
    left_pane_.tabs.set_database(db_.get());
    right_pane_.tabs.set_database(db_.get());
    left_pane_.tabs.set_pane_id("left");
    right_pane_.tabs.set_pane_id("right");
    change_directory(left_pane_, left_pane_.tabs.active_tab().path);
    change_directory(right_pane_, right_pane_.tabs.active_tab().path);
    active_pane_ = active_pane_->id == PaneIdentifier::Left ? &left_pane_ : &right_pane_;
    log_event(__func__, "Swapped panes");
}

PaneState& GridFireApp::pane(PaneIdentifier id) {
    return id == PaneIdentifier::Left ? left_pane_ : right_pane_;
}

PaneState& GridFireApp::active_pane() {
    return *active_pane_;
}

PaneState& GridFireApp::other_pane() {
    return active_pane_->id == PaneIdentifier::Left ? right_pane_ : left_pane_;
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
