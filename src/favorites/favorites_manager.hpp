#pragma once

#include <string>
#include <vector>

#include <optional>

class FavoritesManager {
public:
    struct FavoritePath {
        std::string collection;
        std::string path;
    };

    FavoritesManager() = default;
    void add_favorite(const FavoritePath& favorite);
    void remove_favorite(const std::string& path);
    bool is_favorite(const std::string& path) const;
    std::optional<FavoritePath> find(const std::string& path) const;
    void toggle_favorite(const std::string& path, const std::string& collection = "Quick Access");
    const std::vector<FavoritePath>& list() const noexcept { return favorites_; }

private:
    std::vector<FavoritePath> favorites_;
};
