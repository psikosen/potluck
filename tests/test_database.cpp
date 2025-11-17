#include "database/sqlite_manager.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    const std::string db_path = "test_gridfire.db";
    std::filesystem::remove(db_path);

    SQLiteManager manager(db_path);
    assert(manager.initialize());
    assert(manager.get_schema_version() == 1);

    manager.execute("INSERT INTO file_access (path, access_count, heat_score) VALUES ('/tmp', 1, 1.0);");

    int count = 0;
    manager.query("SELECT COUNT(*) FROM file_access;", [&](sqlite3_stmt* stmt) {
        count = sqlite3_column_int(stmt, 0);
    });
    assert(count == 1);

    std::cout << "Database tests passed" << std::endl;
    std::filesystem::remove(db_path);
    return 0;
}
