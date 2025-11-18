#include "database/sqlite_manager.hpp"
#include "ui/tab_system.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>


int main() {
    const std::string db_path = "test_tabs.db";
    std::filesystem::remove(db_path);

    {
        SQLiteManager manager(db_path);
        assert(manager.initialize());

        TabSystem system;
        system.set_database(&manager);
        system.set_pane_id("left");
        system.initialize("/tmp");
        system.add_tab("/var", "Var");
        system.active_tab().filter_pattern = "*.log";
        system.active_tab().scroll_position = 42;
        system.persist();

        TabSystem reload;
        reload.set_database(&manager);
        reload.set_pane_id("left");
        reload.initialize("/tmp");
        assert(reload.tab_count() >= 1);
        assert(reload.active_tab().path.size() > 0);

        bool found_persisted = false;
        for (size_t i = 0; i < reload.tab_count(); ++i) {
            auto tab = reload.tab_at(i);
            if (tab && tab->path == "/var") {
                assert(tab->filter_pattern == "*.log");
                assert(tab->scroll_position == 42);
                found_persisted = true;
            }
        }
        assert(found_persisted);

        system.close_tab(system.active_index() == 0 ? 0 : system.active_index());
        system.persist();

        reload.load();
        assert(reload.tab_count() >= 1);
    }

    std::filesystem::remove(db_path);
    std::cout << "Tab system tests passed" << std::endl;
    return 0;
}
