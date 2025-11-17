#pragma once

#include <vector>

#include "favorites/favorites_manager.hpp"

class FavoritesDropdown {
public:
    explicit FavoritesDropdown(FavoritesManager* manager);
    std::vector<FavoritesManager::FavoritePath> items() const;

private:
    FavoritesManager* manager_;
};
