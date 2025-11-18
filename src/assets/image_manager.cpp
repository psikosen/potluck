#include "assets/image_manager.hpp"

#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <system_error>

#include "stb_image.h"

namespace {
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", std::gmtime(&now));
    return buffer;
}

void log_event(const std::string& function, const std::string& message, const std::string& error = "") {
    std::ostringstream oss;
    oss << '{'
        << "\"filename\":\"assets/image_manager.cpp\",";
    oss << "\"timestamp\":\"" << timestamp() << "\",";
    oss << "\"classname\":\"ImageManager\",";
    oss << "\"function\":\"" << function << "\",";
    oss << "\"system_section\":\"assets\",";
    oss << "\"line_num\":0,";
    oss << "\"error\":\"" << error << "\",";
    oss << "\"db_phase\":\"none\",";
    oss << "\"method\":\"NONE\",";
    oss << "\"message\":\"" << message << "\"";
    oss << '}';
    std::cout << oss.str() << std::endl;
    std::cout << "Continuous skepticism" << std::endl;
}

std::string normalized_key(const std::filesystem::path& path) {
    std::error_code ec;
    auto absolute_path = std::filesystem::absolute(path, ec);
    std::filesystem::path normalized = ec ? path : absolute_path;
    normalized = normalized.lexically_normal();
    return normalized.string();
}
}  // namespace

bool ImageManager::load_image(const std::string& path) {
    if (path.empty()) {
        log_event(__func__, "Cannot load image from empty path", "empty-path");
        return false;
    }

    std::filesystem::path fs_path(path);
    const std::string key = normalized_key(fs_path);
    std::filesystem::path normalized_path(key);

    std::error_code ec;
    if (!std::filesystem::exists(normalized_path, ec)) {
        log_event(__func__, "Image not found: " + key, ec ? ec.message() : "not-found");
        return false;
    }

    auto file_time = std::filesystem::last_write_time(normalized_path, ec);
    if (ec) {
        log_event(__func__, "Failed to read metadata: " + key, ec.message());
        return false;
    }

    auto cache_it = cache_.find(key);
    if (cache_it != cache_.end() && cache_it->second.last_write_time == file_time) {
        log_event(__func__, "Cache hit for " + key);
        return true;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* raw_pixels = stbi_load(normalized_path.string().c_str(), &width, &height, &channels, 4);
    if (!raw_pixels) {
        const char* reason = stbi_failure_reason();
        log_event(__func__, "Failed to decode image: " + key, reason ? reason : "decode-error");
        return false;
    }

    ImageData data;
    data.width = width;
    data.height = height;
    data.channels = 4;
    data.pixels.assign(raw_pixels, raw_pixels + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    data.last_write_time = file_time;
    data.loaded_at = std::time(nullptr);

    stbi_image_free(raw_pixels);
    cache_[key] = std::move(data);

    log_event(__func__,
              "Loaded image " + key + " (" + std::to_string(width) + "x" + std::to_string(height) + ")",
              "");
    return true;
}

const ImageManager::ImageData* ImageManager::get(const std::string& path) const {
    if (path.empty()) {
        return nullptr;
    }
    const std::string key = normalized_key(path);
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return nullptr;
    }
    return &it->second;
}

void ImageManager::unload(const std::string& path) {
    if (path.empty()) {
        return;
    }
    const std::string key = normalized_key(path);
    cache_.erase(key);
}

void ImageManager::clear() {
    cache_.clear();
}
