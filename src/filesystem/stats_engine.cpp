#include "filesystem/stats_engine.hpp"

#include <filesystem>

DirectoryStatsEngine::Stats DirectoryStatsEngine::summarize(const std::string& path) const {
    Stats stats;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file()) {
            stats.file_count++;
            stats.total_size += entry.file_size();
        } else if (entry.is_directory()) {
            stats.directory_count++;
        }
    }
    return stats;
}
