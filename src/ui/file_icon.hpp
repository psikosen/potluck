#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "imgui.h"

// Unicode file icons for different file types
class FileIconManager {
public:
    enum class FileCategory {
        Directory,
        Code,
        Text,
        Image,
        Audio,
        Video,
        Archive,
        Document,
        Data,
        Config,
        Executable,
        Unknown
    };

    struct IconInfo {
        const char* icon;        // Unicode icon
        ImVec4 color;            // Icon color
        FileCategory category;
    };

    static const IconInfo& get_icon(const std::string& filename, bool is_directory);
    static FileCategory get_category(const std::string& extension);
    static bool is_text_file(const std::string& extension);
    static bool is_openable_in_editor(const std::string& extension);

private:
    static std::string get_extension(const std::string& filename);
    static void initialize_maps();
    static bool initialized_;
    static std::unordered_map<std::string, IconInfo> extension_icons_;
    static std::unordered_set<std::string> text_extensions_;
    static IconInfo directory_icon_;
    static IconInfo unknown_icon_;
};

