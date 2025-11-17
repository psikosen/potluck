#pragma once

#include <string>
#include <vector>

class FavoritesManager {
public:
    struct FavoritePath {
        std::string collection;
        std::string path;
    };

    FavoritesManager() = default;
    void add_favorite(const FavoritePath& favorite);
    const std::vector<FavoritePath>& list() const noexcept { return favorites_; }

private:
    std::vector<FavoritePath> favorites_;
};
