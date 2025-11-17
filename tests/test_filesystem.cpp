#include "database/sqlite_manager.hpp"
#include "filesystem/filesystem_engine.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

int main() {
    const std::string db_path = "test_gridfire.db";
    std::filesystem::remove(db_path);

    SQLiteManager manager(db_path);
    assert(manager.initialize());

    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "gridfire-test";
    std::filesystem::create_directories(temp_dir);

    std::ofstream(temp_dir / "example.txt") << "sample";

    FileSystemEngine engine(&manager);
    auto entries = engine.scan_directory(temp_dir.string());
    assert(!entries.empty());

    engine.update_heat_score((temp_dir / "example.txt").string());

    std::filesystem::remove_all(temp_dir);
    std::filesystem::remove(db_path);
    std::cout << "Filesystem tests passed" << std::endl;
    return 0;
}
