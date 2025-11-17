#pragma once

#include <string>

class ImageManager {
public:
    ImageManager() = default;
    bool load_image(const std::string& path);
};
