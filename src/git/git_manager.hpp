#pragma once

#include <string>

class GitManager {
public:
    GitManager() = default;
    bool refresh_repository(const std::string& path);
};
