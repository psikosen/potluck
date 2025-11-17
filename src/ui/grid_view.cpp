#include "ui/grid_view.hpp"

#include "filesystem/filesystem_engine.hpp"

void GridView::set_entries(const std::vector<FileEntry>& entries) {
    entries_ = entries;
}
