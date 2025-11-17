#include "favorites/favorites_manager.hpp"

#include <algorithm>
#include <filesystem>

namespace {
bool paths_equal(const std::string& lhs, const std::string& rhs) {
    return std::filesystem::path(lhs) == std::filesystem::path(rhs);
}
}

void FavoritesManager::add_favorite(const FavoritePath& favorite) {
    if (is_favorite(favorite.path)) {
        return;
    }
    favorites_.push_back(favorite);
}

void FavoritesManager::remove_favorite(const std::string& path) {
    favorites_.erase(std::remove_if(favorites_.begin(),
                                    favorites_.end(),
                                    [&](const FavoritePath& fav) { return paths_equal(fav.path, path); }),
                     favorites_.end());
}

bool FavoritesManager::is_favorite(const std::string& path) const {
    return std::any_of(favorites_.begin(), favorites_.end(), [&](const FavoritePath& fav) {
        return paths_equal(fav.path, path);
    });
}

std::optional<FavoritesManager::FavoritePath> FavoritesManager::find(const std::string& path) const {
    auto it = std::find_if(favorites_.begin(), favorites_.end(), [&](const FavoritePath& fav) {
        return paths_equal(fav.path, path);
    });
    if (it == favorites_.end()) {
        return std::nullopt;
    }
    return *it;
}

void FavoritesManager::toggle_favorite(const std::string& path, const std::string& collection) {
    if (is_favorite(path)) {
        remove_favorite(path);
    } else {
        add_favorite({collection, path});
    }
}
