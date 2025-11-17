#pragma once

#include <vector>

struct FileEntry;

class GridView {
public:
    GridView() = default;
    void set_entries(const std::vector<FileEntry>& entries);
    const std::vector<FileEntry>& entries() const noexcept { return entries_; }

private:
    std::vector<FileEntry> entries_;
};
