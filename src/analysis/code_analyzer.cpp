#include "analysis/code_analyzer.hpp"

#include <filesystem>
#include <fstream>
#include <string>

CodeAnalyzer::Result CodeAnalyzer::analyze(const std::string& path) const {
    Result result;
    std::ifstream file(path);
    if (!file.is_open()) {
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        ++result.lines_of_code;
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (!trimmed.empty() && trimmed[0] == '#') {
            ++result.comment_lines;
        }
    }

    result.language = std::filesystem::path(path).extension().string();
    result.complexity = static_cast<double>(result.lines_of_code - result.comment_lines);
    return result;
}
