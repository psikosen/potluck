#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct FileEntry {
    std::string name;
    std::string full_path;
    bool is_directory = false;
    uint64_t size = 0;
    std::time_t modified_time = 0;
    std::time_t accessed_time = 0;
    uint32_t permissions = 0;
    std::string owner;
    std::string group;
    float heat_score = 0.0f;
    int access_count = 0;
    std::string custom_icon_path;
};

class SQLiteManager;

class FileSystemEngine {
public:
    explicit FileSystemEngine(SQLiteManager* db);

    std::vector<FileEntry> scan_directory(const std::string& path);
    FileEntry get_file_info(const std::string& path);

    void update_heat_score(const std::string& path);
    float calculate_heat_score(int access_count, std::time_t last_accessed) const;

private:
    void load_heat_data(FileEntry& entry);
    std::string get_owner_name(uid_t uid);
    std::string get_group_name(gid_t gid);
    void log_event(const std::string& function, const std::string& message, const std::string& error = "") const;

    SQLiteManager* db_;
};
