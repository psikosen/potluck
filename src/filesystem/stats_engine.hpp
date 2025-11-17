#pragma once

#include <cstdint>
#include <string>

class DirectoryStatsEngine {
public:
    struct Stats {
        uint64_t total_size = 0;
        uint64_t file_count = 0;
        uint64_t directory_count = 0;
    };

    DirectoryStatsEngine() = default;
    Stats summarize(const std::string& path) const;
};
