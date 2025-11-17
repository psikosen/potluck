#include "assets/image_manager.hpp"

#include <filesystem>

bool ImageManager::load_image(const std::string& path) {
    return std::filesystem::exists(path);
}
