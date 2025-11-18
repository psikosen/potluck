#include "ui/tab_system.hpp"

#include <cstdio>

#include "imgui.h"

TabSystem::RenderResult TabSystem::render(const std::string& pane_label, bool is_active) {
    RenderResult result;
    if (tabs_.empty()) {
        ensure_default_tab(default_path_);
    }

    std::string tab_bar_id = "Tabs##" + pane_label + pane_id_;
    if (ImGui::BeginTabBar(tab_bar_id.c_str(), ImGuiTabBarFlags_Reorderable)) {
        for (size_t i = 0; i < tabs_.size(); ++i) {
            auto& tab = tabs_[i];
            std::string label = tab.name.empty() ? tab.path : tab.name;
            if (tab.pinned) {
                label = "📌 " + label;
            }

            ImGui::PushStyleColor(ImGuiCol_Tab, tab.color);
            ImGui::PushStyleColor(ImGuiCol_TabActive, tab.color);
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(tab.color.x + 0.1f,
                                                               tab.color.y + 0.1f,
                                                               tab.color.z + 0.1f,
                                                               tab.color.w));

            ImGuiTabItemFlags flags = 0;
            if (tab.modified) {
                flags |= ImGuiTabItemFlags_SetSelected;
            }

            bool open = true;
            if (ImGui::BeginTabItem(label.c_str(), &open, flags)) {
                if (active_index_ != i) {
                    switch_to(i);
                    result.active_changed = true;
                }

                if (ImGui::IsItemActivated() && is_active) {
                    result.path_changed = true;
                    result.new_path = tab.path;
                }

                if (ImGui::BeginPopupContextItem()) {
                    std::string rename_popup = "RenameTab##" + tab.id;
                    if (ImGui::MenuItem("Rename")) {
                        ImGui::OpenPopup(rename_popup.c_str());
                    }
                    if (ImGui::BeginPopup(rename_popup.c_str())) {
                        static char buffer[128];
                        if (ImGui::IsWindowAppearing()) {
                            std::snprintf(buffer, sizeof(buffer), "%s", tab.name.c_str());
                        }
                        if (ImGui::InputText("##rename", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            tab.name = buffer;
                            mark_dirty();
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    if (ImGui::MenuItem(tab.pinned ? "Unpin" : "Pin")) {
                        tab.pinned = !tab.pinned;
                        mark_dirty();
                    }
                    if (ImGui::MenuItem("Duplicate")) {
                        duplicate_tab(i);
                    }
                    if (ImGui::MenuItem("Close")) {
                        close_tab(i);
                        result.tab_closed = true;
                    }
                    if (ImGui::MenuItem("Close Others")) {
                        close_other_tabs(i);
                    }
                    if (ImGui::MenuItem("Close Tabs to the Right")) {
                        close_tabs_to_right(i);
                    }
                    std::string color_popup = "ColorPicker##" + tab.id;
                    if (ImGui::MenuItem("Set Color")) {
                        ImGui::OpenPopup(color_popup.c_str());
                    }
                    if (ImGui::BeginPopup(color_popup.c_str())) {
                        if (ImGui::ColorPicker4("Tab Color", &tab.color.x)) {
                            mark_dirty();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::EndPopup();
                }

                ImGui::EndTabItem();
            }

            ImGui::PopStyleColor(3);

            if (!open && !tab.pinned) {
                close_tab(i);
                result.tab_closed = true;
                if (tabs_.empty()) {
                    ensure_default_tab(default_path_);
                }
                break;
            }
        }

        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) {
            add_tab(default_path_, "New Tab");
            result.path_changed = true;
            result.new_path = active_tab().path;
        }
        ImGui::EndTabBar();
    }

    if (is_active && ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_T)) {
            add_tab(active_tab().path, "New Tab");
            result.path_changed = true;
            result.new_path = active_tab().path;
        } else if (ImGui::IsKeyPressed(ImGuiKey_W) && tabs_.size() > 1) {
            close_tab(active_index_);
            result.tab_closed = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
            previous_tab();
            result.path_changed = true;
            result.new_path = active_tab().path;
        } else if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
            next_tab();
            result.path_changed = true;
            result.new_path = active_tab().path;
        } else {
            for (int digit = 0; digit < 9; ++digit) {
                ImGuiKey key = static_cast<ImGuiKey>(ImGuiKey_1 + digit);
                if (ImGui::IsKeyPressed(key)) {
                    size_t target = static_cast<size_t>(digit);
                    if (target < tabs_.size()) {
                        switch_to(target);
                        result.path_changed = true;
                        result.new_path = active_tab().path;
                    }
                }
            }
        }
    }

    return result;
}
