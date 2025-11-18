#include "ui/tab_system.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>

#include "database/sqlite_manager.hpp"
#include "imgui.h"

namespace {
constexpr const char* kTabTable = "tab_state";

int64_t now_seconds() {
    return static_cast<int64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}
}

TabSystem::TabSystem() = default;

void TabSystem::set_pane_id(std::string pane_id) {
    pane_id_ = std::move(pane_id);
}

void TabSystem::initialize(const std::string& default_path) {
    default_path_ = default_path;
    load();
    ensure_default_tab(default_path);
}

const TabSystem::Tab& TabSystem::active_tab() const {
    return tabs_.at(active_index_);
}

TabSystem::Tab& TabSystem::active_tab() {
    return tabs_.at(active_index_);
}

std::optional<TabSystem::Tab> TabSystem::tab_at(size_t index) const {
    if (index >= tabs_.size()) {
        return std::nullopt;
    }
    return tabs_[index];
}

void TabSystem::add_tab(const std::string& path, std::string name) {
    Tab tab;
    tab.id = generate_id();
    tab.name = name.empty() ? path : name;
    tab.path = path;
    tabs_.push_back(tab);
    switch_to(tabs_.size() - 1);
    mark_dirty();
}

void TabSystem::duplicate_tab(size_t index) {
    if (index >= tabs_.size()) {
        return;
    }
    Tab copy = tabs_[index];
    copy.id = generate_id();
    copy.name += " Copy";
    tabs_.insert(tabs_.begin() + static_cast<long>(index + 1), copy);
    switch_to(index + 1);
    mark_dirty();
}

void TabSystem::close_tab(size_t index) {
    if (tabs_.size() <= 1 || index >= tabs_.size()) {
        return;
    }
    tabs_.erase(tabs_.begin() + static_cast<long>(index));
    if (active_index_ >= tabs_.size()) {
        active_index_ = tabs_.size() - 1;
    }
    mark_dirty();
}

void TabSystem::close_other_tabs(size_t index) {
    if (index >= tabs_.size()) {
        return;
    }
    Tab keep = tabs_[index];
    tabs_.clear();
    tabs_.push_back(keep);
    active_index_ = 0;
    mark_dirty();
}

void TabSystem::close_tabs_to_right(size_t index) {
    if (index >= tabs_.size()) {
        return;
    }
    tabs_.erase(tabs_.begin() + static_cast<long>(index + 1), tabs_.end());
    if (active_index_ >= tabs_.size()) {
        active_index_ = tabs_.size() - 1;
    }
    mark_dirty();
}

void TabSystem::next_tab() {
    if (tabs_.empty()) {
        return;
    }
    active_index_ = (active_index_ + 1) % tabs_.size();
}

void TabSystem::previous_tab() {
    if (tabs_.empty()) {
        return;
    }
    if (active_index_ == 0) {
        active_index_ = tabs_.size() - 1;
    } else {
        --active_index_;
    }
}

void TabSystem::persist() {
    if (!db_ || pane_id_.empty() || !dirty_) {
        return;
    }

    if (!db_->begin_transaction()) {
        return;
    }
    {
        std::string delete_sql = "DELETE FROM " + std::string(kTabTable) + " WHERE pane_id = ?";
        auto stmt = db_->prepare(delete_sql);
        if (stmt) {
            db_->bind_text(stmt, 1, pane_id_);
            sqlite3_step(stmt);
            db_->finalize(stmt);
        }
    }
    std::string insert_sql =
        "INSERT INTO " + std::string(kTabTable) +
        " (id, pane_id, name, path, color, pinned, sort_order, created_at, last_accessed) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    for (size_t i = 0; i < tabs_.size(); ++i) {
        auto stmt = db_->prepare(insert_sql);
        if (!stmt) {
            continue;
        }
        db_->bind_text(stmt, 1, tabs_[i].id);
        db_->bind_text(stmt, 2, pane_id_);
        db_->bind_text(stmt, 3, tabs_[i].name);
        db_->bind_text(stmt, 4, tabs_[i].path);
        db_->bind_text(stmt, 5, color_to_hex(tabs_[i].color));
        db_->bind_int(stmt, 6, tabs_[i].pinned ? 1 : 0);
        db_->bind_int(stmt, 7, static_cast<int>(i));
        auto timestamp = now_seconds();
        db_->bind_int64(stmt, 8, timestamp);
        db_->bind_int64(stmt, 9, timestamp);
        sqlite3_step(stmt);
        db_->finalize(stmt);
    }
    db_->commit();
    dirty_ = false;
}

void TabSystem::load() {
    tabs_.clear();
    active_index_ = 0;
    if (!db_ || pane_id_.empty()) {
        return;
    }

    std::string sql =
        "SELECT id, name, path, color, pinned FROM " + std::string(kTabTable) + " WHERE pane_id = ? ORDER BY sort_order";
    auto stmt = db_->prepare(sql);
    if (!stmt) {
        return;
    }
    db_->bind_text(stmt, 1, pane_id_);

    while (db_->step(stmt)) {
        Tab tab;
        tab.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        tab.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        tab.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (color) {
            tab.color = hex_to_color(color);
        }
        tab.pinned = sqlite3_column_int(stmt, 4) == 1;
        tabs_.push_back(tab);
    }
    db_->finalize(stmt);

    if (tabs_.empty()) {
        ensure_default_tab(default_path_);
    }
}

void TabSystem::ensure_default_tab(const std::string& path) {
    if (!tabs_.empty()) {
        return;
    }
    Tab tab;
    tab.id = generate_id();
    tab.name = path.empty() ? "Home" : path;
    tab.path = path.empty() ? "./" : path;
    tab.color = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    tabs_.push_back(tab);
    active_index_ = 0;
}

void TabSystem::switch_to(size_t index) {
    if (index >= tabs_.size()) {
        return;
    }
    active_index_ = index;
}

std::string TabSystem::color_to_hex(const ImVec4& color) const {
    auto clamp = [](float value) {
        return static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    char buffer[10];
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", clamp(color.x), clamp(color.y), clamp(color.z), clamp(color.w));
    return buffer;
}

ImVec4 TabSystem::hex_to_color(const std::string& hex) const {
    if (hex.size() != 9 || hex[0] != '#') {
        return ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    }
    auto read = [&](size_t index) {
        return static_cast<float>(std::stoi(hex.substr(index, 2), nullptr, 16)) / 255.0f;
    };
    return ImVec4(read(1), read(3), read(5), read(7));
}

std::string TabSystem::generate_id() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 15);
    const char* digits = "0123456789abcdef";
    std::string id(16, '0');
    for (char& ch : id) {
        ch = digits[dist(gen)];
    }
    return id;
}

void TabSystem::mark_dirty() {
    dirty_ = true;
}
