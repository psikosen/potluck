#include "git/git_manager.hpp"

#include <filesystem>

bool GitManager::refresh_repository(const std::string& path) {
    return std::filesystem::exists(std::filesystem::path(path) / ".git");
}
