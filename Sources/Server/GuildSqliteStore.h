#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct sqlite3;
class CGame;

struct GuildData {
    uint32_t guid;
    std::string name;
    std::string master_name;
    std::string created_at;
};

struct GuildMemberData {
    std::string character_name;
    uint32_t guild_guid;
    int guild_rank; // 1 = Master, 2 = Officer, 3 = Member
};

bool EnsureGuildDatabase(sqlite3** db, std::string& outPath, CGame* game = nullptr);
void CloseGuildDatabase(sqlite3* db);

// DB operations
bool CreateGuild(sqlite3* db, const std::string& name, const std::string& master_name, uint32_t& out_guid);
bool DisbandGuild(sqlite3* db, uint32_t guild_guid);
bool AddGuildMember(sqlite3* db, const std::string& character_name, uint32_t guild_guid, int rank);
bool RemoveGuildMember(sqlite3* db, const std::string& character_name);
bool GetMemberInfo(sqlite3* db, const std::string& character_name, uint32_t& out_guid, int& out_rank);
bool GetGuildName(sqlite3* db, uint32_t guild_guid, std::string& out_name);
std::vector<GuildMemberData> GetGuildMembers(sqlite3* db, uint32_t guild_guid);

// Progression & Skills
bool GetGuildProgression(sqlite3* db, uint32_t guild_guid, int& out_level, int& out_gxp);
bool UpdateGuildProgression(sqlite3* db, uint32_t guild_guid, int new_level, int new_gxp);
std::vector<std::pair<int, int>> GetGuildSkills(sqlite3* db, uint32_t guild_guid);
bool SetGuildSkill(sqlite3* db, uint32_t guild_guid, int skill_id, int skill_level);
