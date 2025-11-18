#pragma once

#include <optional>
#include <string>
#include <vector>

#include "imgui.h"

class SQLiteManager;

class TabSystem {
public:
    struct Tab {
        std::string id;
        std::string name;
        std::string path;
        std::string view_mode{"grid"};
        std::string sort_mode{"name"};
        ImVec4 color{0.35f, 0.35f, 0.35f, 1.0f};
        bool pinned = false;
        bool modified = false;
        std::string filter_pattern;
        int scroll_position = 0;
    };

    struct RenderResult {
        bool path_changed = false;
        std::string new_path;
        bool active_changed = false;
        bool tab_closed = false;
    };

    TabSystem();

    void set_database(SQLiteManager* db) { db_ = db; }
    void set_pane_id(std::string pane_id);
    void initialize(const std::string& default_path);

    RenderResult render(const std::string& pane_label, bool is_active);

    const Tab& active_tab() const;
    Tab& active_tab();
    std::optional<Tab> tab_at(size_t index) const;

    void add_tab(const std::string& path, std::string name = {});
    void duplicate_tab(size_t index);
    void close_tab(size_t index);
    void close_other_tabs(size_t index);
    void close_tabs_to_right(size_t index);
    void next_tab();
    void previous_tab();

    void persist();
    void load();
    void mark_dirty();

    size_t tab_count() const { return tabs_.size(); }
    size_t active_index() const { return active_index_; }

private:
    void ensure_default_tab(const std::string& path);
    void switch_to(size_t index);
    std::string color_to_hex(const ImVec4& color) const;
    ImVec4 hex_to_color(const std::string& hex) const;
    std::string generate_id() const;
    SQLiteManager* db_ = nullptr;
    std::string pane_id_;
    std::vector<Tab> tabs_;
    size_t active_index_ = 0;
    std::string default_path_;
    bool dirty_ = false;
};
