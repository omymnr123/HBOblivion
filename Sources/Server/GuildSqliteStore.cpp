#include "GuildSqliteStore.h"
#include "Game.h"
#include "Log.h"
#include "TimeUtils.h"
#include "sqlite3.h"
#include <iostream>

bool EnsureGuildDatabase(sqlite3** db, std::string& outPath, CGame* game)
{
    outPath = "guilds.db";
    
    int rc = sqlite3_open(outPath.c_str(), db);
    if (rc != SQLITE_OK) {
        hb::logger::log("Cannot open database: {}", sqlite3_errmsg(*db));
        return false;
    }

    const char* sql = 
        "CREATE TABLE IF NOT EXISTS guilds ("
        " guid INTEGER PRIMARY KEY AUTOINCREMENT,"
        " name TEXT UNIQUE NOT NULL,"
        " master_name TEXT NOT NULL,"
        " created_at TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS guild_members ("
        " character_name TEXT PRIMARY KEY,"
        " guild_guid INTEGER NOT NULL,"
        " guild_rank INTEGER NOT NULL,"
        " last_login INTEGER NOT NULL DEFAULT 0,"
        " FOREIGN KEY(guild_guid) REFERENCES guilds(guid) ON DELETE CASCADE"
        ");";

    char* errMsg = nullptr;
    rc = sqlite3_exec(*db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        hb::logger::log("SQL error creating guild tables: {}", errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    // Migrations: Add new columns if they don't exist (ignore errors if they do)
    sqlite3_exec(*db, "ALTER TABLE guilds ADD COLUMN guild_level INTEGER NOT NULL DEFAULT 1;", nullptr, nullptr, nullptr);
    sqlite3_exec(*db, "ALTER TABLE guilds ADD COLUMN guild_gxp INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(*db, "ALTER TABLE guild_members ADD COLUMN guild_tokens INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(*db, "ALTER TABLE guild_members ADD COLUMN contribution INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(*db, "ALTER TABLE guild_members ADD COLUMN last_login INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);

    const char* sql_skills = 
        "CREATE TABLE IF NOT EXISTS guild_skills ("
        " guild_guid INTEGER NOT NULL,"
        " skill_id INTEGER NOT NULL,"
        " skill_level INTEGER NOT NULL DEFAULT 0,"
        " PRIMARY KEY(guild_guid, skill_id),"
        " FOREIGN KEY(guild_guid) REFERENCES guilds(guid) ON DELETE CASCADE"
        ");";
    sqlite3_exec(*db, sql_skills, nullptr, nullptr, nullptr);

    // Enable foreign keys
    sqlite3_exec(*db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    return true;
}

void CloseGuildDatabase(sqlite3* db)
{
    if (db) {
        sqlite3_close(db);
    }
}

bool CreateGuild(sqlite3* db, const std::string& name, const std::string& master_name, uint32_t& out_guid)
{
    const char* sql = "INSERT INTO guilds (name, master_name, created_at) VALUES (?, ?, datetime('now'))";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, master_name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        out_guid = static_cast<uint32_t>(sqlite3_last_insert_rowid(db));
        return true;
    }
    return false;
}

bool DisbandGuild(sqlite3* db, uint32_t guild_guid)
{
    const char* sql = "DELETE FROM guilds WHERE guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, guild_guid);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool AddGuildMember(sqlite3* db, const std::string& character_name, uint32_t guild_guid, int rank)
{
    const char* sql = "INSERT OR REPLACE INTO guild_members (character_name, guild_guid, guild_rank) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, character_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, guild_guid);
    sqlite3_bind_int(stmt, 3, rank);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool RemoveGuildMember(sqlite3* db, const std::string& character_name)
{
    const char* sql = "DELETE FROM guild_members WHERE character_name = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, character_name.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

bool GetMemberInfo(sqlite3* db, const std::string& character_name, uint32_t& out_guid, int& out_rank)
{
    const char* sql = "SELECT guild_guid, guild_rank FROM guild_members WHERE character_name = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, character_name.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_guid = sqlite3_column_int(stmt, 0);
        out_rank = sqlite3_column_int(stmt, 1);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool GetGuildName(sqlite3* db, uint32_t guild_guid, std::string& out_name)
{
    const char* sql = "SELECT name FROM guilds WHERE guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, guild_guid);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

std::vector<GuildMemberData> GetGuildMembers(sqlite3* db, uint32_t guild_guid)
{
    std::vector<GuildMemberData> members;
    const char* sql = "SELECT character_name, guild_rank FROM guild_members WHERE guild_guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return members;

    sqlite3_bind_int(stmt, 1, guild_guid);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GuildMemberData data;
        data.character_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        data.guild_guid = guild_guid;
        data.guild_rank = sqlite3_column_int(stmt, 1);
        members.push_back(data);
    }
    sqlite3_finalize(stmt);
    return members;
}

bool GetGuildProgression(sqlite3* db, uint32_t guild_guid, int& out_level, int& out_gxp)
{
    const char* sql = "SELECT guild_level, guild_gxp FROM guilds WHERE guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, guild_guid);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_level = sqlite3_column_int(stmt, 0);
        out_gxp = sqlite3_column_int(stmt, 1);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

bool UpdateGuildProgression(sqlite3* db, uint32_t guild_guid, int new_level, int new_gxp)
{
    const char* sql = "UPDATE guilds SET guild_level = ?, guild_gxp = ? WHERE guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, new_level);
    sqlite3_bind_int(stmt, 2, new_gxp);
    sqlite3_bind_int(stmt, 3, guild_guid);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<std::pair<int, int>> GetGuildSkills(sqlite3* db, uint32_t guild_guid)
{
    std::vector<std::pair<int, int>> skills;
    const char* sql = "SELECT skill_id, skill_level FROM guild_skills WHERE guild_guid = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return skills;

    sqlite3_bind_int(stmt, 1, guild_guid);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        skills.push_back({sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1)});
    }
    sqlite3_finalize(stmt);
    return skills;
}

bool SetGuildSkill(sqlite3* db, uint32_t guild_guid, int skill_id, int skill_level)
{
    const char* sql = "INSERT OR REPLACE INTO guild_skills (guild_guid, skill_id, skill_level) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, guild_guid);
    sqlite3_bind_int(stmt, 2, skill_id);
    sqlite3_bind_int(stmt, 3, skill_level);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}
