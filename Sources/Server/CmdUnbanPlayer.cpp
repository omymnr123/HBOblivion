#include "CmdUnbanPlayer.h"
#include "Game.h"
#include "ServerConfig.h"
#include "ServerConsole.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "AccountSqliteStore.h"
#include "sqlite3.h"
#include <cstdio>
#include <string>

void CmdUnbanPlayer::execute(CGame* game, const char* args)
{
    if (args == nullptr || args[0] == '\0')
    {
        hb::console::error("Usage: unban-player <account_name>");
        return;
    }

    char account_name[64] = {};
    std::sscanf(args, "%s", account_name);

    if (account_name[0] == '\0')
    {
        hb::console::error("Invalid arguments. Usage: unban-player <account_name>");
        return;
    }

    sqlite3* db = nullptr;
    std::string dbPath;
    
    if (EnsureAccountDatabase(account_name, &db, dbPath))
    {
        sqlite3_exec(db, "DELETE FROM account_bans;", nullptr, nullptr, nullptr);
        sqlite3_close(db);

        hb::console::success("Account '{}' has been UNBANNED successfully.", account_name);
        hb::logger::log<hb::log_channel::commands>("unban-player: account '{}' unbanned", account_name);
    }
    else
    {
        hb::console::error("Could not find account database for '{}'.", account_name);
    }
}