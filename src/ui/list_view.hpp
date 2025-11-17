#pragma once

#include <vector>

struct FileEntry;

class ListView {
public:
    ListView() = default;
    void set_entries(const std::vector<FileEntry>& entries);
    const std::vector<FileEntry>& entries() const noexcept { return entries_; }

private:
    std::vector<FileEntry> entries_;
};
