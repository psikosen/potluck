#include "ui/list_view.hpp"

#include "filesystem/filesystem_engine.hpp"

void ListView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}
