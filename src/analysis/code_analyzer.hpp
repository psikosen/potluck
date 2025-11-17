#pragma once

#include <string>
#include <vector>

class CodeAnalyzer {
public:
    struct Result {
        std::string language;
        int lines_of_code = 0;
        int comment_lines = 0;
        double complexity = 0.0;
    };

    CodeAnalyzer() = default;
    Result analyze(const std::string& path) const;
};
