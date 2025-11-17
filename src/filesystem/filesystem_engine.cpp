#include "filesystem/filesystem_engine.hpp"

#include "database/sqlite_manager.hpp"

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cmath>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace {
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", std::gmtime(&now));
    return buffer;
}
}

FileSystemEngine::FileSystemEngine(SQLiteManager* db) : db_(db) {}

std::vector<FileEntry> FileSystemEngine::scan_directory(const std::string& path) {
    std::vector<FileEntry> entries;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            FileEntry info = get_file_info(entry.path().string());
            load_heat_data(info);
            entries.push_back(info);
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        log_event(__func__, "scan failure", ex.what());
    }

    return entries;
}

FileEntry FileSystemEngine::get_file_info(const std::string& path) {
    FileEntry entry;
    entry.full_path = path;
    entry.name = std::filesystem::path(path).filename().string();
    entry.is_directory = std::filesystem::is_directory(path);

    struct stat st {};
    if (stat(path.c_str(), &st) == 0) {
        entry.size = static_cast<uint64_t>(st.st_size);
        entry.modified_time = st.st_mtime;
        entry.accessed_time = st.st_atime;
        entry.permissions = st.st_mode;
        entry.owner = get_owner_name(st.st_uid);
        entry.group = get_group_name(st.st_gid);
    }

    return entry;
}

void FileSystemEngine::load_heat_data(FileEntry& entry) {
    if (!db_) {
        return;
    }

    sqlite3_stmt* stmt = db_->prepare("SELECT access_count, heat_score, custom_icon_path FROM file_access WHERE path = ?;");
    if (!stmt) {
        return;
    }

    db_->bind_text(stmt, 1, entry.full_path);
    if (db_->step(stmt)) {
        entry.access_count = sqlite3_column_int(stmt, 0);
        entry.heat_score = static_cast<float>(sqlite3_column_double(stmt, 1));
        const unsigned char* icon = sqlite3_column_text(stmt, 2);
        if (icon) {
            entry.custom_icon_path = reinterpret_cast<const char*>(icon);
        }
    }
    db_->finalize(stmt);
}

void FileSystemEngine::update_heat_score(const std::string& path) {
    if (!db_) {
        return;
    }

    std::time_t now = std::time(nullptr);
    float new_heat = calculate_heat_score(1, now);

    const char* sql = R"SQL(
        INSERT INTO file_access (path, access_count, last_accessed, heat_score)
        VALUES (?, 1, ?, ?)
        ON CONFLICT(path) DO UPDATE SET
            access_count = file_access.access_count + 1,
            last_accessed = excluded.last_accessed,
            heat_score = excluded.heat_score
    )SQL";

    sqlite3_stmt* stmt = db_->prepare(sql);
    if (!stmt) {
        return;
    }

    db_->bind_text(stmt, 1, path);
    db_->bind_int64(stmt, 2, now);
    db_->bind_double(stmt, 3, new_heat);

    sqlite3_step(stmt);
    db_->finalize(stmt);
}

float FileSystemEngine::calculate_heat_score(int access_count, std::time_t last_accessed) const {
    std::time_t now = std::time(nullptr);
    double hours_since_access = static_cast<double>(now - last_accessed) / 3600.0;
    double decay = std::exp(-hours_since_access / 48.0);
    double frequency_boost = std::log(static_cast<double>(access_count) + 1.0);
    return static_cast<float>(decay * frequency_boost);
}

std::string FileSystemEngine::get_owner_name(uid_t uid) {
    passwd* pw = getpwuid(uid);
    if (pw && pw->pw_name) {
        return pw->pw_name;
    }
    return std::to_string(uid);
}

std::string FileSystemEngine::get_group_name(gid_t gid) {
    group* gr = getgrgid(gid);
    if (gr && gr->gr_name) {
        return gr->gr_name;
    }
    return std::to_string(gid);
}

void FileSystemEngine::log_event(const std::string& function, const std::string& message, const std::string& error) const {
    std::ostringstream oss;
    oss << '{'
        << "\"filename\":\"filesystem/filesystem_engine.cpp\",";
    oss << "\"timestamp\":\"" << timestamp() << "\",";
    oss << "\"classname\":\"FileSystemEngine\",";
    oss << "\"function\":\"" << function << "\",";
    oss << "\"system_section\":\"filesystem\",";
    oss << "\"line_num\":0,";
    oss << "\"error\":\"" << error << "\",";
    oss << "\"db_phase\":\"none\",";
    oss << "\"method\":\"NONE\",";
    oss << "\"message\":\"" << message << "\"";
    oss << '}';
    std::cout << oss.str() << std::endl;
    std::cout << "Continuous skepticism" << std::endl;
}
