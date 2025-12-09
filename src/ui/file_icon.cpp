#include "ui/file_icon.hpp"

#include <algorithm>
#include <cctype>

bool FileIconManager::initialized_ = false;
std::unordered_map<std::string, FileIconManager::IconInfo> FileIconManager::extension_icons_;
std::unordered_set<std::string> FileIconManager::text_extensions_;
FileIconManager::IconInfo FileIconManager::directory_icon_;
FileIconManager::IconInfo FileIconManager::unknown_icon_;

void FileIconManager::initialize_maps() {
    if (initialized_) return;

    // Use simple ASCII text icons that will always render
    // Directory icon
    directory_icon_ = {"[D]", ImVec4(1.0f, 0.85f, 0.4f, 1.0f), FileCategory::Directory};

    // Unknown file icon
    unknown_icon_ = {"[.]", ImVec4(0.7f, 0.7f, 0.7f, 1.0f), FileCategory::Unknown};

    // Code files - green tones
    ImVec4 code_color(0.4f, 0.9f, 0.5f, 1.0f);
    extension_icons_["cpp"] = {"<C>", code_color, FileCategory::Code};
    extension_icons_["c"] = {"<C>", code_color, FileCategory::Code};
    extension_icons_["h"] = {"<H>", code_color, FileCategory::Code};
    extension_icons_["hpp"] = {"<H>", code_color, FileCategory::Code};
    extension_icons_["py"] = {"<P>", ImVec4(0.3f, 0.7f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["js"] = {"JS", ImVec4(0.95f, 0.85f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["ts"] = {"TS", ImVec4(0.2f, 0.5f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["jsx"] = {"JX", ImVec4(0.4f, 0.85f, 0.95f, 1.0f), FileCategory::Code};
    extension_icons_["tsx"] = {"TX", ImVec4(0.2f, 0.5f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["java"] = {"JV", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["rs"] = {"RS", ImVec4(0.9f, 0.5f, 0.2f, 1.0f), FileCategory::Code};
    extension_icons_["go"] = {"GO", ImVec4(0.3f, 0.8f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["rb"] = {"RB", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["php"] = {"PH", ImVec4(0.5f, 0.5f, 0.8f, 1.0f), FileCategory::Code};
    extension_icons_["cs"] = {"C#", ImVec4(0.5f, 0.3f, 0.8f, 1.0f), FileCategory::Code};
    extension_icons_["swift"] = {"SW", ImVec4(0.95f, 0.5f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["kt"] = {"KT", ImVec4(0.5f, 0.4f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["scala"] = {"SC", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["lua"] = {"LU", ImVec4(0.2f, 0.3f, 0.7f, 1.0f), FileCategory::Code};
    extension_icons_["sh"] = {"SH", ImVec4(0.5f, 0.9f, 0.5f, 1.0f), FileCategory::Code};
    extension_icons_["bash"] = {"SH", ImVec4(0.5f, 0.9f, 0.5f, 1.0f), FileCategory::Code};
    extension_icons_["zsh"] = {"SH", ImVec4(0.5f, 0.9f, 0.5f, 1.0f), FileCategory::Code};
    extension_icons_["sql"] = {"DB", ImVec4(0.3f, 0.6f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["r"] = {"<R>", ImVec4(0.3f, 0.5f, 0.9f, 1.0f), FileCategory::Code};

    // Web/Markup files
    extension_icons_["html"] = {"HT", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["htm"] = {"HT", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Code};
    extension_icons_["css"] = {"CS", ImVec4(0.3f, 0.5f, 0.9f, 1.0f), FileCategory::Code};
    extension_icons_["scss"] = {"SS", ImVec4(0.85f, 0.5f, 0.7f, 1.0f), FileCategory::Code};
    extension_icons_["sass"] = {"SA", ImVec4(0.85f, 0.5f, 0.7f, 1.0f), FileCategory::Code};
    extension_icons_["less"] = {"LE", ImVec4(0.3f, 0.4f, 0.6f, 1.0f), FileCategory::Code};

    // Text files - cyan tones
    ImVec4 text_color(0.5f, 0.9f, 0.95f, 1.0f);
    extension_icons_["txt"] = {"TX", text_color, FileCategory::Text};
    extension_icons_["md"] = {"MD", ImVec4(0.3f, 0.5f, 0.8f, 1.0f), FileCategory::Text};
    extension_icons_["markdown"] = {"MD", ImVec4(0.3f, 0.5f, 0.8f, 1.0f), FileCategory::Text};
    extension_icons_["rst"] = {"RS", ImVec4(0.5f, 0.6f, 0.7f, 1.0f), FileCategory::Text};
    extension_icons_["log"] = {"LG", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), FileCategory::Text};

    // Image files - magenta tones
    ImVec4 image_color(0.95f, 0.5f, 0.8f, 1.0f);
    extension_icons_["png"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["jpg"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["jpeg"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["gif"] = {"GF", image_color, FileCategory::Image};
    extension_icons_["bmp"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["webp"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["svg"] = {"SV", ImVec4(0.95f, 0.7f, 0.3f, 1.0f), FileCategory::Image};
    extension_icons_["ico"] = {"IC", image_color, FileCategory::Image};
    extension_icons_["tiff"] = {"IM", image_color, FileCategory::Image};
    extension_icons_["psd"] = {"PS", ImVec4(0.3f, 0.5f, 0.9f, 1.0f), FileCategory::Image};

    // Audio files - purple tones
    ImVec4 audio_color(0.7f, 0.5f, 0.95f, 1.0f);
    extension_icons_["mp3"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["wav"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["ogg"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["flac"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["aac"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["m4a"] = {"AU", audio_color, FileCategory::Audio};
    extension_icons_["wma"] = {"AU", audio_color, FileCategory::Audio};

    // Video files - red tones
    ImVec4 video_color(0.95f, 0.4f, 0.4f, 1.0f);
    extension_icons_["mp4"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["avi"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["mkv"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["mov"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["wmv"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["flv"] = {"VD", video_color, FileCategory::Video};
    extension_icons_["webm"] = {"VD", video_color, FileCategory::Video};

    // Archive files - brown/orange tones
    ImVec4 archive_color(0.9f, 0.7f, 0.4f, 1.0f);
    extension_icons_["zip"] = {"ZP", archive_color, FileCategory::Archive};
    extension_icons_["tar"] = {"TR", archive_color, FileCategory::Archive};
    extension_icons_["gz"] = {"GZ", archive_color, FileCategory::Archive};
    extension_icons_["bz2"] = {"BZ", archive_color, FileCategory::Archive};
    extension_icons_["xz"] = {"XZ", archive_color, FileCategory::Archive};
    extension_icons_["7z"] = {"7Z", archive_color, FileCategory::Archive};
    extension_icons_["rar"] = {"RR", archive_color, FileCategory::Archive};

    // Document files - blue tones
    ImVec4 doc_color(0.3f, 0.6f, 0.95f, 1.0f);
    extension_icons_["pdf"] = {"PD", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), FileCategory::Document};
    extension_icons_["doc"] = {"DC", doc_color, FileCategory::Document};
    extension_icons_["docx"] = {"DC", doc_color, FileCategory::Document};
    extension_icons_["odt"] = {"DC", doc_color, FileCategory::Document};
    extension_icons_["xls"] = {"XL", ImVec4(0.3f, 0.7f, 0.4f, 1.0f), FileCategory::Document};
    extension_icons_["xlsx"] = {"XL", ImVec4(0.3f, 0.7f, 0.4f, 1.0f), FileCategory::Document};
    extension_icons_["ppt"] = {"PT", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Document};
    extension_icons_["pptx"] = {"PT", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Document};

    // Data files - teal tones
    ImVec4 data_color(0.3f, 0.85f, 0.8f, 1.0f);
    extension_icons_["json"] = {"{}", data_color, FileCategory::Data};
    extension_icons_["xml"] = {"<>", data_color, FileCategory::Data};
    extension_icons_["yaml"] = {"YM", data_color, FileCategory::Data};
    extension_icons_["yml"] = {"YM", data_color, FileCategory::Data};
    extension_icons_["toml"] = {"TM", data_color, FileCategory::Data};
    extension_icons_["csv"] = {",,", data_color, FileCategory::Data};
    extension_icons_["ini"] = {"##", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), FileCategory::Config};
    extension_icons_["cfg"] = {"##", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), FileCategory::Config};
    extension_icons_["conf"] = {"##", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), FileCategory::Config};
    extension_icons_["env"] = {"$$", ImVec4(0.8f, 0.7f, 0.3f, 1.0f), FileCategory::Config};

    // Executables - orange tones
    ImVec4 exe_color(0.95f, 0.6f, 0.3f, 1.0f);
    extension_icons_["exe"] = {"EX", exe_color, FileCategory::Executable};
    extension_icons_["bin"] = {"BN", exe_color, FileCategory::Executable};
    extension_icons_["app"] = {"AP", exe_color, FileCategory::Executable};
    extension_icons_["deb"] = {"DB", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), FileCategory::Executable};
    extension_icons_["rpm"] = {"RP", ImVec4(0.9f, 0.3f, 0.3f, 1.0f), FileCategory::Executable};
    extension_icons_["dmg"] = {"DM", ImVec4(0.7f, 0.7f, 0.7f, 1.0f), FileCategory::Executable};
    extension_icons_["iso"] = {"IS", ImVec4(0.7f, 0.7f, 0.7f, 1.0f), FileCategory::Executable};

    // Special files
    extension_icons_["gitignore"] = {"GI", ImVec4(0.9f, 0.5f, 0.3f, 1.0f), FileCategory::Config};
    extension_icons_["dockerignore"] = {"DI", ImVec4(0.3f, 0.6f, 0.9f, 1.0f), FileCategory::Config};
    extension_icons_["makefile"] = {"MK", ImVec4(0.5f, 0.8f, 0.5f, 1.0f), FileCategory::Config};
    extension_icons_["cmake"] = {"CM", ImVec4(0.3f, 0.6f, 0.9f, 1.0f), FileCategory::Config};
    extension_icons_["dockerfile"] = {"DK", ImVec4(0.3f, 0.6f, 0.95f, 1.0f), FileCategory::Config};
    extension_icons_["license"] = {"LI", ImVec4(0.9f, 0.8f, 0.3f, 1.0f), FileCategory::Text};
    extension_icons_["readme"] = {"RD", ImVec4(0.3f, 0.5f, 0.9f, 1.0f), FileCategory::Text};
    extension_icons_["lock"] = {"LK", ImVec4(0.6f, 0.6f, 0.6f, 1.0f), FileCategory::Data};

    // Populate text extensions for "open in editor" feature
    text_extensions_ = {
        "txt", "md", "markdown", "rst", "log",
        "cpp", "c", "h", "hpp", "cc", "cxx", "hxx",
        "py", "pyw", "pyi",
        "js", "jsx", "ts", "tsx", "mjs", "cjs",
        "java", "kt", "kts", "scala",
        "rs", "go", "rb", "php",
        "cs", "fs", "vb",
        "swift", "m", "mm",
        "lua", "pl", "pm", "tcl",
        "sh", "bash", "zsh", "fish", "ps1", "bat", "cmd",
        "sql",
        "r", "R",
        "html", "htm", "css", "scss", "sass", "less",
        "xml", "json", "yaml", "yml", "toml", "ini", "cfg", "conf", "env",
        "gitignore", "dockerignore", "editorconfig",
        "makefile", "cmake",
        "dockerfile",
        "license", "readme",
        "csv", "tsv"
    };

    initialized_ = true;
}

std::string FileIconManager::get_extension(const std::string& filename) {
    // Handle special filenames without extension
    std::string lower_name = filename;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Special filenames
    if (lower_name == "makefile" || lower_name == "gnumakefile") return "makefile";
    if (lower_name == "cmakelists.txt") return "cmake";
    if (lower_name == "dockerfile") return "dockerfile";
    if (lower_name == "license" || lower_name == "license.txt" || lower_name == "license.md") return "license";
    if (lower_name == "readme" || lower_name == "readme.txt" || lower_name == "readme.md") return "readme";
    if (lower_name.find(".lock") != std::string::npos) return "lock";

    // Find last dot
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos || dot_pos == 0 || dot_pos == filename.size() - 1) {
        return "";
    }

    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

const FileIconManager::IconInfo& FileIconManager::get_icon(const std::string& filename, bool is_directory) {
    initialize_maps();

    if (is_directory) {
        return directory_icon_;
    }

    std::string ext = get_extension(filename);
    auto it = extension_icons_.find(ext);
    if (it != extension_icons_.end()) {
        return it->second;
    }

    return unknown_icon_;
}

FileIconManager::FileCategory FileIconManager::get_category(const std::string& extension) {
    initialize_maps();

    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    auto it = extension_icons_.find(ext);
    if (it != extension_icons_.end()) {
        return it->second.category;
    }
    return FileCategory::Unknown;
}

bool FileIconManager::is_text_file(const std::string& extension) {
    initialize_maps();

    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return text_extensions_.count(ext) > 0;
}

bool FileIconManager::is_openable_in_editor(const std::string& extension) {
    return is_text_file(extension);
}

