#pragma once

#include <string>

class ThemeManager {
public:
    struct Theme {
        std::string name;
    };

    ThemeManager() = default;
    void apply_theme(const Theme& theme);
    const Theme& current_theme() const noexcept { return theme_; }

private:
    Theme theme_{"Inferno"};
};
