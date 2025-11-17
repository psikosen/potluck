#pragma once

#include <functional>
#include <memory>
#include <sqlite3.h>
#include <string>

class SQLiteManager {
public:
    explicit SQLiteManager(const std::string& db_path);
    ~SQLiteManager();

    bool initialize();
    bool execute(const std::string& sql);

    using RowCallback = std::function<void(sqlite3_stmt*)>;
    bool query(const std::string& sql, RowCallback callback);

    sqlite3_stmt* prepare(const std::string& sql);
    bool bind_text(sqlite3_stmt* stmt, int index, const std::string& value);
    bool bind_int(sqlite3_stmt* stmt, int index, int value);
    bool bind_int64(sqlite3_stmt* stmt, int index, int64_t value);
    bool bind_double(sqlite3_stmt* stmt, int index, double value);

    bool step(sqlite3_stmt* stmt);
    void finalize(sqlite3_stmt* stmt);

    bool begin_transaction();
    bool commit();
    bool rollback();

    bool create_tables();
    int get_schema_version();
    bool set_schema_version(int version);

    sqlite3* raw() const noexcept { return db_; }

private:
    void log_sql_error(const std::string& sql, const std::string& error_message) const;

    std::string db_path_;
    sqlite3* db_ = nullptr;
    bool in_transaction_ = false;
};
