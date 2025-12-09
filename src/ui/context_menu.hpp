#pragma once

#include <optional>
#include <string>

#include "filesystem/filesystem_engine.hpp"

// Context menu actions for files/directories
enum class ContextMenuAction {
    None,
    Open,
    OpenInEditor,
    OpenInTerminal,
    Copy,
    Cut,
    Paste,
    Delete,
    Rename,
    NewFolder,
    NewFile,
    Properties,
    CopyPath,
    CopyToOtherPane,
    MoveToOtherPane
};

struct ContextMenuResult {
    ContextMenuAction action = ContextMenuAction::None;
    std::string target_path;
    std::optional<std::string> new_name;  // For rename operations
};

class ContextMenu {
public:
    ContextMenu() = default;

    // Render context menu for a file entry
    // Returns the selected action (if any)
    ContextMenuResult render(const FileEntry& entry, const std::string& popup_id);

    // Render context menu for empty space (background)
    ContextMenuResult render_background(const std::string& current_path, const std::string& popup_id);

    // Check if clipboard has a path
    bool has_clipboard() const { return !clipboard_path_.empty(); }
    const std::string& clipboard_path() const { return clipboard_path_; }
    bool is_clipboard_cut() const { return clipboard_is_cut_; }

    // Set clipboard
    void set_clipboard(const std::string& path, bool is_cut);
    void clear_clipboard();

private:
    std::string clipboard_path_;
    bool clipboard_is_cut_ = false;
};

