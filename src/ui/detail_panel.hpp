#pragma once

#include <optional>

#include "filesystem/filesystem_engine.hpp"

class DetailPanel {
public:
    DetailPanel() = default;
    void set_selection(const std::optional<FileEntry>& entry);
    void render() const;
    const std::optional<FileEntry>& selection() const noexcept { return selection_; }

private:
    std::optional<FileEntry> selection_;
};
