#pragma once

#include <string>

#include "imgui.h"

class ThemeManager {
public:
    struct Theme {
        std::string name;
        ImVec4 background_color{0.08f, 0.08f, 0.09f, 1.0f};
        ImVec4 surface_color{0.13f, 0.13f, 0.14f, 1.0f};
        ImVec4 accent_color{0.89f, 0.44f, 0.17f, 1.0f};
        ImVec4 accent_hover_color{0.98f, 0.55f, 0.28f, 1.0f};
        ImVec4 text_color{0.93f, 0.93f, 0.93f, 1.0f};
        ImVec4 subtle_text_color{0.65f, 0.65f, 0.68f, 1.0f};
        ImVec4 heat_cold_color{0.20f, 0.33f, 0.54f, 0.90f};
        ImVec4 heat_hot_color{0.95f, 0.27f, 0.08f, 0.95f};
        ImVec4 selection_color{0.98f, 0.63f, 0.21f, 0.55f};
        ImVec4 selection_text_color{0.05f, 0.05f, 0.05f, 1.0f};
    };

    ThemeManager();
    void apply_theme(const Theme& theme);
    const Theme& current_theme() const noexcept { return theme_; }

private:
    void apply_to_imgui(const Theme& theme);

private:
    Theme theme_{"Inferno"};
};
