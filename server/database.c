#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"

int init_database() {
    sqlite3 *db;
    int rc = sqlite3_open("chat.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    char *sql = "CREATE TABLE IF NOT EXISTS users ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "username TEXT UNIQUE NOT NULL,"
                "password_hash TEXT NOT NULL);"
                "CREATE TABLE IF NOT EXISTS messages ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "sender TEXT NOT NULL,"
                "receiver TEXT,"
                "message TEXT NOT NULL,"
                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";
    
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    sqlite3_close(db);
    return rc == SQLITE_OK ? 0 : -1;
}

int register_user(const char *username, const char *password_hash) {
    sqlite3 *db;
    if (sqlite3_open("chat.db", &db) != SQLITE_OK) return -1;
    
    char *sql = "INSERT INTO users(username, password_hash) VALUES(?, ?)";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return rc == SQLITE_DONE ? 0 : -1;
}