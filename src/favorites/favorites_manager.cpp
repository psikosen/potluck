#include "favorites/favorites_manager.hpp"

void FavoritesManager::add_favorite(const FavoritePath& favorite) {
    favorites_.push_back(favorite);
}
