#pragma once

#include <optional>
#include <string>
#include <vector>

#include "favorites/favorites_manager.hpp"

class FavoritesDropdown {
public:
    struct Interaction {
        std::optional<std::string> selected_path;
        bool toggle_current = false;
    };

    explicit FavoritesDropdown(FavoritesManager* manager);
    std::vector<FavoritesManager::FavoritePath> items() const;
    Interaction render(const std::string& current_path) const;

private:
    FavoritesManager* manager_;
};
