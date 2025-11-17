#include "ui/theme.hpp"

#include "imgui.h"

namespace {
ThemeManager::Theme inferno_theme() {
    ThemeManager::Theme theme{"Inferno"};
    theme.background_color = ImVec4(0.05f, 0.05f, 0.06f, 1.0f);
    theme.surface_color = ImVec4(0.11f, 0.11f, 0.13f, 1.0f);
    theme.accent_color = ImVec4(0.98f, 0.38f, 0.11f, 1.0f);
    theme.accent_hover_color = ImVec4(1.0f, 0.48f, 0.20f, 1.0f);
    theme.text_color = ImVec4(0.92f, 0.92f, 0.95f, 1.0f);
    theme.subtle_text_color = ImVec4(0.65f, 0.65f, 0.70f, 1.0f);
    theme.heat_cold_color = ImVec4(0.16f, 0.33f, 0.59f, 0.9f);
    theme.heat_hot_color = ImVec4(0.98f, 0.23f, 0.11f, 0.95f);
    theme.selection_color = ImVec4(0.95f, 0.54f, 0.12f, 0.60f);
    theme.selection_text_color = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    return theme;
}
}

ThemeManager::ThemeManager() {
    apply_theme(inferno_theme());
}

void ThemeManager::apply_theme(const Theme& theme) {
    theme_ = theme;
    apply_to_imgui(theme_);
}

void ThemeManager::apply_to_imgui(const Theme& theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = theme.text_color;
    colors[ImGuiCol_TextDisabled] = theme.subtle_text_color;
    colors[ImGuiCol_WindowBg] = theme.background_color;
    colors[ImGuiCol_ChildBg] = theme.surface_color;
    colors[ImGuiCol_PopupBg] = theme.surface_color;
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBg] = theme.surface_color;
    colors[ImGuiCol_FrameBgHovered] = theme.accent_hover_color;
    colors[ImGuiCol_FrameBgActive] = theme.accent_color;
    colors[ImGuiCol_TitleBg] = theme.surface_color;
    colors[ImGuiCol_TitleBgActive] = theme.surface_color;
    colors[ImGuiCol_TitleBgCollapsed] = theme.surface_color;
    colors[ImGuiCol_CheckMark] = theme.accent_color;
    colors[ImGuiCol_SliderGrab] = theme.accent_color;
    colors[ImGuiCol_SliderGrabActive] = theme.accent_hover_color;
    colors[ImGuiCol_Button] = theme.accent_color;
    colors[ImGuiCol_ButtonHovered] = theme.accent_hover_color;
    colors[ImGuiCol_ButtonActive] = theme.accent_color;
    colors[ImGuiCol_Header] = theme.selection_color;
    colors[ImGuiCol_HeaderHovered] = theme.accent_hover_color;
    colors[ImGuiCol_HeaderActive] = theme.accent_color;
    colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.24f, 0.26f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = theme.accent_color;
    colors[ImGuiCol_ResizeGripHovered] = theme.accent_hover_color;
    colors[ImGuiCol_ResizeGripActive] = theme.accent_hover_color;
    colors[ImGuiCol_Tab] = theme.surface_color;
    colors[ImGuiCol_TabHovered] = theme.accent_hover_color;
    colors[ImGuiCol_TabActive] = theme.accent_color;
    colors[ImGuiCol_TabUnfocused] = theme.surface_color;
    colors[ImGuiCol_TabUnfocusedActive] = theme.accent_color;
}

