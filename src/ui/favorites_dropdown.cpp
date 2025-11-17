#include "ui/favorites_dropdown.hpp"

FavoritesDropdown::FavoritesDropdown(FavoritesManager* manager) : manager_(manager) {}

std::vector<FavoritesManager::FavoritePath> FavoritesDropdown::items() const {
    if (!manager_) {
        return {};
    }
    return manager_->list();
}
