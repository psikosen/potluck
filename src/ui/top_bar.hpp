#pragma once

#include <string>

class TopBar {
public:
    TopBar() = default;
    void set_title(const std::string& title);
    const std::string& title() const noexcept { return title_; }

private:
    std::string title_ = "GridFire";
};
