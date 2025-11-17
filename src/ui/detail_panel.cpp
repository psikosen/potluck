#include "ui/detail_panel.hpp"

#include "filesystem/filesystem_engine.hpp"

void DetailPanel::set_selection(const std::optional<FileEntry>& entry) {
    selection_ = entry;
}
