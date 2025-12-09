#include "ui/context_menu.hpp"

#include <algorithm>
#include <filesystem>

#include "imgui.h"
#include "ui/file_icon.hpp"

ContextMenuResult ContextMenu::render(const FileEntry& entry, const std::string& popup_id) {
    ContextMenuResult result;
    result.target_path = entry.full_path;

    if (ImGui::BeginPopup(popup_id.c_str())) {
        ImGui::SeparatorText(entry.name.c_str());

        // Open action
        if (entry.is_directory) {
            if (ImGui::MenuItem("Open")) {
                result.action = ContextMenuAction::Open;
            }
            if (ImGui::MenuItem("Open in Terminal")) {
                result.action = ContextMenuAction::OpenInTerminal;
            }
        } else {
            if (ImGui::MenuItem("Open")) {
                result.action = ContextMenuAction::Open;
            }

            // Check if file can be opened in text editor
            std::string ext;
            size_t dot_pos = entry.name.rfind('.');
            if (dot_pos != std::string::npos && dot_pos < entry.name.size() - 1) {
                ext = entry.name.substr(dot_pos + 1);
            }

            if (FileIconManager::is_openable_in_editor(ext)) {
                if (ImGui::MenuItem("Open in Editor")) {
                    result.action = ContextMenuAction::OpenInEditor;
                }
            }
        }

        ImGui::Separator();

        // Clipboard operations
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            result.action = ContextMenuAction::Copy;
        }
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            result.action = ContextMenuAction::Cut;
        }

        // Paste is only available if we have something in clipboard
        ImGui::BeginDisabled(!has_clipboard());
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            result.action = ContextMenuAction::Paste;
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        // Pane operations
        if (ImGui::MenuItem("Copy to Other Pane ->")) {
            result.action = ContextMenuAction::CopyToOtherPane;
        }
        if (ImGui::MenuItem("Move to Other Pane ->")) {
            result.action = ContextMenuAction::MoveToOtherPane;
        }

        ImGui::Separator();

        // Rename
        if (ImGui::MenuItem("Rename", "F2")) {
            result.action = ContextMenuAction::Rename;
        }

        // Delete
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        if (ImGui::MenuItem("Delete", "Del")) {
            result.action = ContextMenuAction::Delete;
        }
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Copy path
        if (ImGui::MenuItem("Copy Path")) {
            result.action = ContextMenuAction::CopyPath;
        }

        // Properties
        if (ImGui::MenuItem("Properties")) {
            result.action = ContextMenuAction::Properties;
        }

        ImGui::EndPopup();
    }

    return result;
}

ContextMenuResult ContextMenu::render_background(const std::string& current_path, const std::string& popup_id) {
    ContextMenuResult result;
    result.target_path = current_path;

    if (ImGui::BeginPopup(popup_id.c_str())) {
        ImGui::SeparatorText("Actions");

        // Paste (if clipboard has content)
        ImGui::BeginDisabled(!has_clipboard());
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            result.action = ContextMenuAction::Paste;
        }
        ImGui::EndDisabled();

        ImGui::Separator();

        // New folder
        if (ImGui::MenuItem("New Folder", "Ctrl+Shift+N")) {
            result.action = ContextMenuAction::NewFolder;
        }

        // New file
        if (ImGui::MenuItem("New File", "Ctrl+N")) {
            result.action = ContextMenuAction::NewFile;
        }

        ImGui::Separator();

        // Open terminal here
        if (ImGui::MenuItem("Open Terminal Here")) {
            result.action = ContextMenuAction::OpenInTerminal;
        }

        ImGui::Separator();

        // Copy path
        if (ImGui::MenuItem("Copy Path")) {
            result.action = ContextMenuAction::CopyPath;
        }

        // Properties
        if (ImGui::MenuItem("Properties")) {
            result.action = ContextMenuAction::Properties;
        }

        ImGui::EndPopup();
    }

    return result;
}

void ContextMenu::set_clipboard(const std::string& path, bool is_cut) {
    clipboard_path_ = path;
    clipboard_is_cut_ = is_cut;
}

void ContextMenu::clear_clipboard() {
    clipboard_path_.clear();
    clipboard_is_cut_ = false;
}

