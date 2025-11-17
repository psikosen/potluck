#pragma once

#include <string>
#include <optional>

class TopBar {
public:
    struct Interaction {
        std::optional<std::string> requested_path;
        bool refresh_requested = false;
        bool toggle_view_mode = false;
    };

    TopBar() = default;
    void set_title(const std::string& title);
    void set_path(const std::string& path);
    Interaction render(bool is_grid_mode, bool busy);
    const std::string& title() const noexcept { return title_; }

private:
    std::string title_ = "GridFire";
    std::string path_buffer_;
    bool path_dirty_ = false;
};
