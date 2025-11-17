#include "ui/favorites_dropdown.hpp"

#include "imgui.h"

FavoritesDropdown::FavoritesDropdown(FavoritesManager* manager) : manager_(manager) {}

std::vector<FavoritesManager::FavoritePath> FavoritesDropdown::items() const {
    if (!manager_) {
        return {};
    }
    return manager_->list();
}

FavoritesDropdown::Interaction FavoritesDropdown::render(const std::string& current_path) const {
    Interaction interaction;
    if (!manager_) {
        return interaction;
    }

    const auto& favorites = manager_->list();
    std::string current_label = manager_->is_favorite(current_path) ? "★ Favorite" : "☆ Not favorite";
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo("Favorites", current_label.c_str())) {
        for (const auto& favorite : favorites) {
            std::string label = favorite.collection + " — " + favorite.path;
            if (ImGui::Selectable(label.c_str())) {
                interaction.selected_path = favorite.path;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button(manager_->is_favorite(current_path) ? "Remove" : "Add")) {
        interaction.toggle_current = true;
    }

    return interaction;
}
