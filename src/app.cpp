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
#include "ui/file_icon.hpp"
#include "ui/theme.hpp"
#include "ui/top_bar.hpp"

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
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

    ImGui::Begin("Navigation", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
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

    ImGui::Begin("Workspace", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
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
    if (pane_interaction.left.context_action.action != ContextMenuAction::None) {
        handle_context_action(left_pane_, pane_interaction.left.context_action);
    }
    if (pane_interaction.left.drop_received) {
        handle_drop(left_pane_, pane_interaction.left.dropped_path, pane_interaction.left.drop_target_path);
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
    if (pane_interaction.right.context_action.action != ContextMenuAction::None) {
        handle_context_action(right_pane_, pane_interaction.right.context_action);
    }
    if (pane_interaction.right.drop_received) {
        handle_drop(right_pane_, pane_interaction.right.dropped_path, pane_interaction.right.drop_target_path);
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

    // Render dialogs
    if (show_rename_dialog_) {
        ImGui::OpenPopup("Rename");
    }
    if (ImGui::BeginPopupModal("Rename", &show_rename_dialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Rename: %s", std::filesystem::path(dialog_target_path_).filename().c_str());
        ImGui::Separator();
        ImGui::InputText("New name", dialog_input_buffer_, sizeof(dialog_input_buffer_));
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            std::filesystem::path old_path(dialog_target_path_);
            std::filesystem::path new_path = old_path.parent_path() / dialog_input_buffer_;
            std::error_code ec;
            std::filesystem::rename(old_path, new_path, ec);
            if (!ec) {
                status_message_ = "Renamed to " + std::string(dialog_input_buffer_);
                if (dialog_pane_) {
                    dialog_pane_->refresh_requested = true;
                }
            } else {
                status_message_ = "Rename failed: " + ec.message();
            }
            show_rename_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_rename_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (show_delete_dialog_) {
        ImGui::OpenPopup("Confirm Delete");
    }
    if (ImGui::BeginPopupModal("Confirm Delete", &show_delete_dialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete: %s", dialog_target_path_.c_str());
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "This action cannot be undone!");
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            std::error_code ec;
            std::filesystem::remove_all(dialog_target_path_, ec);
            if (!ec) {
                status_message_ = "Deleted: " + std::filesystem::path(dialog_target_path_).filename().string();
                if (dialog_pane_) {
                    dialog_pane_->refresh_requested = true;
                }
            } else {
                status_message_ = "Delete failed: " + ec.message();
            }
            show_delete_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_delete_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (show_new_folder_dialog_) {
        ImGui::OpenPopup("New Folder");
    }
    if (ImGui::BeginPopupModal("New Folder", &show_new_folder_dialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Create folder in: %s", dialog_target_path_.c_str());
        ImGui::Separator();
        ImGui::InputText("Folder name", dialog_input_buffer_, sizeof(dialog_input_buffer_));
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            std::filesystem::path new_folder = std::filesystem::path(dialog_target_path_) / dialog_input_buffer_;
            std::error_code ec;
            std::filesystem::create_directory(new_folder, ec);
            if (!ec) {
                status_message_ = "Created folder: " + std::string(dialog_input_buffer_);
                if (dialog_pane_) {
                    dialog_pane_->refresh_requested = true;
                }
            } else {
                status_message_ = "Create folder failed: " + ec.message();
            }
            show_new_folder_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_new_folder_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (show_new_file_dialog_) {
        ImGui::OpenPopup("New File");
    }
    if (ImGui::BeginPopupModal("New File", &show_new_file_dialog_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Create file in: %s", dialog_target_path_.c_str());
        ImGui::Separator();
        ImGui::InputText("File name", dialog_input_buffer_, sizeof(dialog_input_buffer_));
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            std::filesystem::path new_file = std::filesystem::path(dialog_target_path_) / dialog_input_buffer_;
            std::ofstream ofs(new_file);
            if (ofs.good()) {
                status_message_ = "Created file: " + std::string(dialog_input_buffer_);
                if (dialog_pane_) {
                    dialog_pane_->refresh_requested = true;
                }
            } else {
                status_message_ = "Create file failed";
            }
            show_new_file_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            show_new_file_dialog_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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
    std::string trimmed = command;
    trimmed.erase(trimmed.begin(),
                  std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                  trimmed.end());

    if (trimmed.rfind("cd", 0) == 0 && (trimmed.size() == 2 || std::isspace(static_cast<unsigned char>(trimmed[2])))) {
        std::string arg;
        if (trimmed.size() > 2) {
            arg = trimmed.substr(3);
            arg.erase(arg.begin(),
                      std::find_if(arg.begin(), arg.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            arg.erase(std::find_if(arg.rbegin(), arg.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                      arg.end());
        }

        std::filesystem::path target_path;
        if (arg.empty()) {
            if (const char* home = std::getenv("HOME")) {
                target_path = std::filesystem::path(home);
            } else {
                target_path = std::filesystem::current_path();
            }
        } else {
            std::filesystem::path base(active_pane().current_path);
            std::filesystem::path arg_path(arg);
            if (arg_path.is_absolute()) {
                target_path = arg_path;
            } else {
                target_path = base / arg_path;
            }
        }

        std::string target = target_path.lexically_normal().string();
        std::string previous_path = active_pane().current_path;
        change_directory(active_pane(), target);

        command_panel_->set_last_command(trimmed);
        command_panel_->set_last_output("");
        bool changed = active_pane().current_path != previous_path;
        last_command_exit_code_ = changed ? 0 : 1;
        last_command_message_ = changed ? "Changed directory" : "Failed to change directory";
        status_message_ = last_command_message_ + " to " + active_pane().current_path;
        return;
    }

    CommandResult result = command_executor_->run(command, active_pane().current_path);
    command_panel_->set_last_command(command);
    command_panel_->set_last_output(result.output);
    last_command_exit_code_ = result.exit_code;
    last_command_message_ = result.exit_code == 0 ? "Command executed" : "Command failed";
    status_message_ = last_command_message_ + ": " + command;
}

void GridFireApp::handle_context_action(PaneState& pane, const ContextMenuResult& action) {
    switch (action.action) {
        case ContextMenuAction::Open:
            if (std::filesystem::is_directory(action.target_path)) {
                change_directory(pane, action.target_path);
            } else {
                // Open file with system default
                std::string command = "xdg-open '" + action.target_path + "' &";
                std::system(command.c_str());
                status_message_ = "Opened: " + action.target_path;
            }
            break;

        case ContextMenuAction::OpenInEditor:
            open_in_editor(action.target_path);
            break;

        case ContextMenuAction::OpenInTerminal:
            open_in_terminal(action.target_path);
            break;

        case ContextMenuAction::Copy:
            pane.context_menu.set_clipboard(action.target_path, false);
            status_message_ = "Copied to clipboard: " + std::filesystem::path(action.target_path).filename().string();
            break;

        case ContextMenuAction::Cut:
            pane.context_menu.set_clipboard(action.target_path, true);
            status_message_ = "Cut to clipboard: " + std::filesystem::path(action.target_path).filename().string();
            break;

        case ContextMenuAction::Paste:
            paste_from_clipboard(pane, action.target_path);
            break;

        case ContextMenuAction::Delete:
            delete_entry(pane, action.target_path);
            break;

        case ContextMenuAction::Rename:
            rename_entry(pane, action.target_path);
            break;

        case ContextMenuAction::NewFolder:
            create_new_folder(pane, action.target_path);
            break;

        case ContextMenuAction::NewFile:
            create_new_file(pane, action.target_path);
            break;

        case ContextMenuAction::CopyPath:
            copy_path_to_clipboard(action.target_path);
            break;

        case ContextMenuAction::CopyToOtherPane:
            if (pane.selected_entry) {
                copy_between_panes(pane, other_pane(), false);
            }
            break;

        case ContextMenuAction::MoveToOtherPane:
            if (pane.selected_entry) {
                copy_between_panes(pane, other_pane(), true);
            }
            break;

        case ContextMenuAction::Properties:
            status_message_ = "Properties: " + action.target_path;
            break;

        case ContextMenuAction::None:
            break;
    }
}

void GridFireApp::handle_drop(PaneState& /*pane*/, const std::string& source_path, const std::string& target_dir) {
    std::filesystem::path source(source_path);
    std::filesystem::path target = std::filesystem::path(target_dir) / source.filename();

    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        status_message_ = "Target already exists: " + target.string();
        return;
    }

    try {
        if (std::filesystem::is_directory(source)) {
            std::filesystem::copy(source, target, std::filesystem::copy_options::recursive, ec);
        } else {
            std::filesystem::copy_file(source, target, ec);
        }

        if (!ec) {
            status_message_ = "Copied " + source.filename().string() + " to " + target_dir;
            log_event(__func__, status_message_);
            left_pane_.refresh_requested = true;
            right_pane_.refresh_requested = true;
        } else {
            status_message_ = "Copy failed: " + ec.message();
        }
    } catch (const std::filesystem::filesystem_error& error) {
        status_message_ = std::string("Copy failed: ") + error.what();
    }
}

void GridFireApp::open_in_editor(const std::string& path) {
    // Try common editors in order of preference
    const char* editors[] = {
        "code",           // VS Code
        "gedit",          // GNOME editor
        "kate",           // KDE editor
        "xed",            // Linux Mint editor
        "mousepad",       // Xfce editor
        "leafpad",        // Simple editor
        "nano",           // Terminal editor
        "vim",            // Vim
        nullptr
    };

    std::string editor_cmd;
    for (int i = 0; editors[i] != nullptr; ++i) {
        std::string check_cmd = std::string("which ") + editors[i] + " > /dev/null 2>&1";
        if (std::system(check_cmd.c_str()) == 0) {
            editor_cmd = editors[i];
            break;
        }
    }

    if (editor_cmd.empty()) {
        // Fallback to xdg-open
        editor_cmd = "xdg-open";
    }

    std::string command = editor_cmd + " '" + path + "' &";
    std::system(command.c_str());
    status_message_ = "Opened in editor: " + path;
    log_event(__func__, status_message_);
}

void GridFireApp::open_in_terminal(const std::string& path) {
    std::string dir_path = path;
    if (!std::filesystem::is_directory(path)) {
        dir_path = std::filesystem::path(path).parent_path().string();
    }

    // Try common terminal emulators
    const char* terminals[] = {
        "gnome-terminal --working-directory=",
        "konsole --workdir ",
        "xfce4-terminal --working-directory=",
        "mate-terminal --working-directory=",
        "xterm -e 'cd ",
        nullptr
    };

    std::string terminal_cmd;
    for (int i = 0; terminals[i] != nullptr; ++i) {
        std::string check_cmd = std::string("which ") + std::string(terminals[i]).substr(0, std::string(terminals[i]).find(' ')) + " > /dev/null 2>&1";
        if (std::system(check_cmd.c_str()) == 0) {
            terminal_cmd = terminals[i];
            break;
        }
    }

    if (!terminal_cmd.empty()) {
        std::string command;
        if (terminal_cmd.find("xterm") != std::string::npos) {
            command = terminal_cmd + dir_path + " && bash' &";
        } else {
            command = terminal_cmd + "'" + dir_path + "' &";
        }
        std::system(command.c_str());
        status_message_ = "Opened terminal in: " + dir_path;
    } else {
        status_message_ = "No terminal emulator found";
    }
    log_event(__func__, status_message_);
}

void GridFireApp::delete_entry(PaneState& pane, const std::string& path) {
    dialog_target_path_ = path;
    dialog_pane_ = &pane;
    show_delete_dialog_ = true;
}

void GridFireApp::rename_entry(PaneState& pane, const std::string& path) {
    dialog_target_path_ = path;
    dialog_pane_ = &pane;
    std::string filename = std::filesystem::path(path).filename().string();
    std::strncpy(dialog_input_buffer_, filename.c_str(), sizeof(dialog_input_buffer_) - 1);
    dialog_input_buffer_[sizeof(dialog_input_buffer_) - 1] = '\0';
    show_rename_dialog_ = true;
}

void GridFireApp::create_new_folder(PaneState& pane, const std::string& parent_path) {
    dialog_target_path_ = parent_path;
    dialog_pane_ = &pane;
    std::strncpy(dialog_input_buffer_, "New Folder", sizeof(dialog_input_buffer_) - 1);
    dialog_input_buffer_[sizeof(dialog_input_buffer_) - 1] = '\0';
    show_new_folder_dialog_ = true;
}

void GridFireApp::create_new_file(PaneState& pane, const std::string& parent_path) {
    dialog_target_path_ = parent_path;
    dialog_pane_ = &pane;
    std::strncpy(dialog_input_buffer_, "new_file.txt", sizeof(dialog_input_buffer_) - 1);
    dialog_input_buffer_[sizeof(dialog_input_buffer_) - 1] = '\0';
    show_new_file_dialog_ = true;
}

void GridFireApp::copy_path_to_clipboard(const std::string& path) {
    // Use xclip or xsel to copy to system clipboard
    std::string command = "echo -n '" + path + "' | xclip -selection clipboard 2>/dev/null || echo -n '" + path + "' | xsel --clipboard 2>/dev/null";
    std::system(command.c_str());
    status_message_ = "Path copied to clipboard: " + path;
}

void GridFireApp::paste_from_clipboard(PaneState& /*pane*/, const std::string& target_dir) {
    // Check both panes for clipboard content
    std::string clipboard_path;
    bool is_cut = false;

    if (left_pane_.context_menu.has_clipboard()) {
        clipboard_path = left_pane_.context_menu.clipboard_path();
        is_cut = left_pane_.context_menu.is_clipboard_cut();
    } else if (right_pane_.context_menu.has_clipboard()) {
        clipboard_path = right_pane_.context_menu.clipboard_path();
        is_cut = right_pane_.context_menu.is_clipboard_cut();
    }

    if (clipboard_path.empty()) {
        status_message_ = "Clipboard is empty";
        return;
    }

    std::filesystem::path source(clipboard_path);
    std::filesystem::path target = std::filesystem::path(target_dir) / source.filename();

    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        status_message_ = "Target already exists: " + target.string();
        return;
    }

    try {
        if (is_cut) {
            std::filesystem::rename(source, target, ec);
            if (!ec) {
                status_message_ = "Moved " + source.filename().string() + " to " + target_dir;
                left_pane_.context_menu.clear_clipboard();
                right_pane_.context_menu.clear_clipboard();
            }
        } else {
            if (std::filesystem::is_directory(source)) {
                std::filesystem::copy(source, target, std::filesystem::copy_options::recursive, ec);
            } else {
                std::filesystem::copy_file(source, target, ec);
            }
            if (!ec) {
                status_message_ = "Pasted " + source.filename().string() + " to " + target_dir;
            }
        }

        if (ec) {
            status_message_ = "Paste failed: " + ec.message();
        } else {
            log_event(__func__, status_message_);
            left_pane_.refresh_requested = true;
            right_pane_.refresh_requested = true;
        }
    } catch (const std::filesystem::filesystem_error& error) {
        status_message_ = std::string("Paste failed: ") + error.what();
    }
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
