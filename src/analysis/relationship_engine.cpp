#include "analysis/relationship_engine.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
struct FileDescriptor {
    std::filesystem::path path;
    std::string path_string;
    std::string stem;
    std::string extension;
    std::string directory;
    bool is_test = false;
};

std::string timestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", std::gmtime(&now));
    return buffer;
}

void log_event(const std::string& function, const std::string& message, const std::string& error = "") {
    std::ostringstream oss;
    oss << '{'
        << "\"filename\":\"analysis/relationship_engine.cpp\",";
    oss << "\"timestamp\":\"" << timestamp() << "\",";
    oss << "\"classname\":\"RelationshipEngine\",";
    oss << "\"function\":\"" << function << "\",";
    oss << "\"system_section\":\"analysis\",";
    oss << "\"line_num\":0,";
    oss << "\"error\":\"" << error << "\",";
    oss << "\"db_phase\":\"none\",";
    oss << "\"method\":\"NONE\",";
    oss << "\"message\":\"" << message << "\"";
    oss << '}';
    std::cout << oss.str() << std::endl;
    std::cout << "Continuous skepticism" << std::endl;
}

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ends_with(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

size_t common_prefix_length(const std::string& lhs, const std::string& rhs) {
    size_t count = 0;
    const size_t limit = std::min(lhs.size(), rhs.size());
    while (count < limit && lhs[count] == rhs[count]) {
        ++count;
    }
    return count;
}

double extension_affinity(const std::string& lhs, const std::string& rhs) {
    static const std::unordered_set<std::string> header_ext = {".h", ".hpp", ".hh"};
    static const std::unordered_set<std::string> source_ext = {".c", ".cc", ".cpp", ".cxx", ".mm"};

    const std::string left = to_lower(lhs);
    const std::string right = to_lower(rhs);
    if ((header_ext.count(left) && source_ext.count(right)) || (header_ext.count(right) && source_ext.count(left))) {
        return 0.95;
    }
    if (left == right && !left.empty()) {
        return 0.75;
    }
    if ((left == ".py" && right == ".py") || (left == ".js" && right == ".ts")) {
        return 0.7;
    }
    return 0.6;
}

bool looks_like_test(const std::string& stem) {
    const std::string lower = to_lower(stem);
    return lower.find("test") != std::string::npos || ends_with(lower, "_spec");
}

std::string canonical_test_target(const std::string& stem) {
    const std::string lower = to_lower(stem);
    const std::vector<std::string> suffixes = {"_test", "_tests", "_spec", "test"};
    for (const auto& suffix : suffixes) {
        if (ends_with(lower, suffix) && stem.size() > suffix.size()) {
            return stem.substr(0, stem.size() - suffix.size());
        }
    }
    return stem;
}

void register_relationship(const FileDescriptor& lhs,
                           const FileDescriptor& rhs,
                           const std::string& type,
                           double strength,
                           std::vector<RelationshipEngine::Relationship>& out,
                           std::unordered_set<std::string>& emitted) {
    RelationshipEngine::Relationship rel;
    rel.file_a = lhs.path_string;
    rel.file_b = rhs.path_string;
    rel.type = type;
    rel.strength = std::clamp(strength, 0.0, 1.0);

    const std::string first = std::min(rel.file_a, rel.file_b);
    const std::string second = std::max(rel.file_a, rel.file_b);
    const std::string key = type + '|' + first + '|' + second;
    if (emitted.insert(key).second) {
        out.push_back(rel);
    }
}
}  // namespace

std::vector<RelationshipEngine::Relationship> RelationshipEngine::detect(const std::vector<std::string>& files) const {
    std::vector<Relationship> relationships;
    if (files.empty()) {
        log_event(__func__, "No files provided for relationship detection");
        return relationships;
    }

    std::vector<FileDescriptor> descriptors;
    descriptors.reserve(files.size());
    std::unordered_set<std::string> seen_paths;
    for (const auto& file : files) {
        if (file.empty()) {
            log_event(__func__, "Skipping empty path entry", "empty-path");
            continue;
        }

        std::filesystem::path raw_path(file);
        std::filesystem::path normalized = raw_path.lexically_normal();
        const std::string normalized_str = normalized.string();
        if (normalized_str.empty()) {
            log_event(__func__, "Path normalization resulted in empty value", "normalization-error");
            continue;
        }
        if (!seen_paths.insert(normalized_str).second) {
            log_event(__func__, "Skipping duplicate file entry: " + normalized_str);
            continue;
        }

        FileDescriptor descriptor;
        descriptor.path = normalized;
        descriptor.path_string = normalized_str;
        descriptor.stem = normalized.stem().string();
        descriptor.extension = normalized.extension().string();
        descriptor.directory = normalized.parent_path().string();
        descriptor.is_test = looks_like_test(descriptor.stem);
        descriptors.push_back(std::move(descriptor));
    }

    if (descriptors.empty()) {
        log_event(__func__, "No analyzable files after normalization", "no-data");
        return relationships;
    }

    std::unordered_map<std::string, std::vector<const FileDescriptor*>> by_stem;
    std::unordered_map<std::string, std::vector<const FileDescriptor*>> non_test_by_stem;
    std::unordered_map<std::string, std::vector<const FileDescriptor*>> by_directory;
    for (const auto& descriptor : descriptors) {
        if (!descriptor.stem.empty()) {
            by_stem[descriptor.stem].push_back(&descriptor);
            if (!descriptor.is_test) {
                non_test_by_stem[descriptor.stem].push_back(&descriptor);
            }
        }
        by_directory[descriptor.directory].push_back(&descriptor);
    }

    std::unordered_set<std::string> emitted;

    for (const auto& [stem, group] : by_stem) {
        if (group.size() < 2) {
            continue;
        }
        for (size_t i = 0; i < group.size(); ++i) {
            for (size_t j = i + 1; j < group.size(); ++j) {
                const double score = extension_affinity(group[i]->extension, group[j]->extension);
                register_relationship(*group[i], *group[j], "companion", score, relationships, emitted);
            }
        }
    }

    for (const auto& descriptor : descriptors) {
        if (!descriptor.is_test) {
            continue;
        }
        const std::string target_stem = canonical_test_target(descriptor.stem);
        auto it = non_test_by_stem.find(target_stem);
        if (it == non_test_by_stem.end()) {
            continue;
        }
        for (const FileDescriptor* candidate : it->second) {
            register_relationship(*candidate, descriptor, "validation", 0.85, relationships, emitted);
        }
    }

    for (auto& [directory, group] : by_directory) {
        if (group.size() < 2) {
            continue;
        }
        std::sort(group.begin(), group.end(), [](const FileDescriptor* lhs, const FileDescriptor* rhs) {
            if (lhs->stem == rhs->stem) {
                return lhs->extension < rhs->extension;
            }
            return lhs->stem < rhs->stem;
        });

        for (size_t i = 1; i < group.size(); ++i) {
            const FileDescriptor* previous = group[i - 1];
            const FileDescriptor* current = group[i];
            if (previous->path_string == current->path_string) {
                continue;
            }

            const std::string left = to_lower(previous->stem);
            const std::string right = to_lower(current->stem);
            const size_t prefix = common_prefix_length(left, right);
            if (prefix >= 3) {
                const double ratio = static_cast<double>(prefix) /
                                     static_cast<double>(std::min(left.size(), right.size()));
                register_relationship(*previous,
                                      *current,
                                      "shared-module",
                                      0.6 + 0.4 * ratio,
                                      relationships,
                                      emitted);
            } else if (!previous->extension.empty() && previous->extension == current->extension) {
                register_relationship(*previous, *current, "shared-extension", 0.55, relationships, emitted);
            }
        }
    }

    std::ostringstream summary;
    summary << "Analyzed " << descriptors.size() << " files, produced " << relationships.size() << " relationships";
    log_event(__func__, summary.str());
    return relationships;
}
