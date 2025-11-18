#pragma once

#include <ctime>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class ImageManager {
public:
    ImageManager() = default;
    struct ImageData {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<unsigned char> pixels;
        std::filesystem::file_time_type last_write_time{};
        std::time_t loaded_at = 0;
    };

    bool load_image(const std::string& path);
    const ImageData* get(const std::string& path) const;
    void unload(const std::string& path);
    void clear();

private:
    std::unordered_map<std::string, ImageData> cache_;
};
