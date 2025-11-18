#include "database/sqlite_manager.hpp"

#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::string current_timestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", std::gmtime(&now));
    return buffer;
}

void log_message(const std::string& filename,
                 const std::string& function,
                 int line,
                 const std::string& message,
                 const std::string& error = "") {
    std::ostringstream oss;
    oss << '{'
        << "\"filename\":\"" << filename << "\",";
    oss << "\"timestamp\":\"" << current_timestamp() << "\",";
    oss << "\"classname\":\"SQLiteManager\",";
    oss << "\"function\":\"" << function << "\",";
    oss << "\"system_section\":\"database\",";
    oss << "\"line_num\":" << line << ',';
    oss << "\"error\":\"" << error << "\",";
    oss << "\"db_phase\":\"none\",";
    oss << "\"method\":\"NONE\",";
    oss << "\"message\":\"" << message << "\"";
    oss << '}';
    std::cout << oss.str() << std::endl;
    std::cout << "Continuous skepticism" << std::endl;
}
}

SQLiteManager::SQLiteManager(const std::string& db_path) : db_path_(db_path) {}

SQLiteManager::~SQLiteManager() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool SQLiteManager::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        log_sql_error("sqlite3_open", sqlite3_errmsg(db_));
        return false;
    }

    execute("PRAGMA foreign_keys = ON;");
    return create_tables();
}

bool SQLiteManager::execute(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "unknown";
        sqlite3_free(err_msg);
        log_sql_error(sql, error);
        return false;
    }
    return true;
}

bool SQLiteManager::query(const std::string& sql, RowCallback callback) {
    sqlite3_stmt* stmt = prepare(sql);
    if (!stmt) {
        return false;
    }

    while (step(stmt)) {
        callback(stmt);
    }

    finalize(stmt);
    return true;
}

sqlite3_stmt* SQLiteManager::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        log_sql_error(sql, sqlite3_errmsg(db_));
        return nullptr;
    }
    return stmt;
}

bool SQLiteManager::bind_text(sqlite3_stmt* stmt, int index, const std::string& value) {
    return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool SQLiteManager::bind_int(sqlite3_stmt* stmt, int index, int value) {
    return sqlite3_bind_int(stmt, index, value) == SQLITE_OK;
}

bool SQLiteManager::bind_int64(sqlite3_stmt* stmt, int index, int64_t value) {
    return sqlite3_bind_int64(stmt, index, value) == SQLITE_OK;
}

bool SQLiteManager::bind_double(sqlite3_stmt* stmt, int index, double value) {
    return sqlite3_bind_double(stmt, index, value) == SQLITE_OK;
}

bool SQLiteManager::step(sqlite3_stmt* stmt) {
    int rc = sqlite3_step(stmt);
    return rc == SQLITE_ROW;
}

void SQLiteManager::finalize(sqlite3_stmt* stmt) {
    sqlite3_finalize(stmt);
}

bool SQLiteManager::begin_transaction() {
    if (in_transaction_) {
        return false;
    }
    if (execute("BEGIN TRANSACTION;")) {
        in_transaction_ = true;
        return true;
    }
    return false;
}

bool SQLiteManager::commit() {
    if (!in_transaction_) {
        return false;
    }
    bool result = execute("COMMIT;");
    in_transaction_ = false;
    return result;
}

bool SQLiteManager::rollback() {
    if (!in_transaction_) {
        return false;
    }
    bool result = execute("ROLLBACK;");
    in_transaction_ = false;
    return result;
}

int SQLiteManager::get_schema_version() {
    int version = 0;
    query("SELECT version FROM schema_version LIMIT 1;", [&](sqlite3_stmt* stmt) {
        version = sqlite3_column_int(stmt, 0);
    });
    return version;
}

bool SQLiteManager::set_schema_version(int version) {
    std::string sql = "UPDATE schema_version SET version = ?;";
    sqlite3_stmt* stmt = prepare(sql);
    if (!stmt) {
        return false;
    }
    bind_int(stmt, 1, version);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    finalize(stmt);
    return result;
}

bool SQLiteManager::create_tables() {
    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS file_access (
            id INTEGER PRIMARY KEY,
            path TEXT UNIQUE NOT NULL,
            access_count INTEGER DEFAULT 0,
            last_accessed INTEGER,
            heat_score REAL,
            custom_icon_path TEXT
        );

        CREATE TABLE IF NOT EXISTS code_metadata (
            id INTEGER PRIMARY KEY,
            file_path TEXT UNIQUE NOT NULL,
            language TEXT,
            lines_of_code INTEGER,
            comment_lines INTEGER,
            complexity_score REAL,
            imports TEXT,
            functions TEXT,
            classes TEXT,
            last_analyzed INTEGER,
            FOREIGN KEY (file_path) REFERENCES file_access(path)
        );

        CREATE TABLE IF NOT EXISTS file_relationships (
            id INTEGER PRIMARY KEY,
            file_a TEXT NOT NULL,
            file_b TEXT NOT NULL,
            relationship_type TEXT,
            strength REAL,
            metadata TEXT,
            UNIQUE(file_a, file_b, relationship_type)
        );

        CREATE TABLE IF NOT EXISTS favorite_collections (
            id INTEGER PRIMARY KEY,
            name TEXT UNIQUE NOT NULL,
            icon TEXT,
            sort_order INTEGER,
            created_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS favorite_paths (
            id INTEGER PRIMARY KEY,
            collection_id INTEGER NOT NULL,
            path TEXT NOT NULL,
            alias TEXT,
            added_at INTEGER,
            FOREIGN KEY (collection_id) REFERENCES favorite_collections(id)
        );

        CREATE TABLE IF NOT EXISTS tab_state (
            id TEXT PRIMARY KEY,
            pane_id TEXT,
            name TEXT,
            path TEXT,
            color TEXT,
            pinned INTEGER,
            sort_order INTEGER,
            view_mode TEXT,
            sort_mode TEXT,
            filter_pattern TEXT,
            scroll_position INTEGER,
            created_at INTEGER,
            last_accessed INTEGER
        );

        CREATE TABLE IF NOT EXISTS tab_groups (
            id INTEGER PRIMARY KEY,
            name TEXT UNIQUE,
            created_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS tab_group_members (
            group_id INTEGER,
            tab_id TEXT,
            FOREIGN KEY (group_id) REFERENCES tab_groups(id),
            FOREIGN KEY (tab_id) REFERENCES tab_state(id)
        );

        CREATE TABLE IF NOT EXISTS git_repos (
            id INTEGER PRIMARY KEY,
            path TEXT UNIQUE NOT NULL,
            remote_url TEXT,
            current_branch TEXT,
            last_fetched INTEGER,
            uncommitted_changes INTEGER,
            unpushed_commits INTEGER
        );

        CREATE TABLE IF NOT EXISTS directory_stats (
            path TEXT PRIMARY KEY,
            total_size INTEGER,
            file_count INTEGER,
            directory_count INTEGER,
            largest_file TEXT,
            oldest_file TEXT,
            newest_file TEXT,
            cached_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS command_history (
            id INTEGER PRIMARY KEY,
            command TEXT NOT NULL,
            executed_at INTEGER,
            exit_code INTEGER,
            working_directory TEXT
        );

        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER PRIMARY KEY
        );

        INSERT OR IGNORE INTO schema_version (version) VALUES (1);
    )SQL";

    return execute(schema);
}

void SQLiteManager::log_sql_error(const std::string& sql, const std::string& error_message) const {
    log_message(__FILE__, __func__, __LINE__, sql, error_message);
}
