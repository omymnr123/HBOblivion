#include "CmdBanPlayer.h"
#include "Game.h"
#include "ServerConfig.h"
#include "ServerConsole.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "AccountSqliteStore.h"
#include "sqlite3.h"
#include "Client.h"
#include <ctime>
#include <cstdio>
#include <string>
#include <sstream>
#include <algorithm>

void CmdBanPlayer::execute(CGame* game, const char* args)
{
    if (args == nullptr || args[0] == '\0')
    {
        hb::console::error("Usage: ban-player <character_name> <duration_seconds|perma> [reason]");
        return;
    }

    std::string arg_str(args);
    std::stringstream ss(arg_str);

    std::string char_name_str;
    std::string duration_token;

    if (!(ss >> char_name_str >> duration_token))
    {
        hb::console::error("Usage: ban-player <character_name> <duration_seconds|perma> [reason]");
        return;
    }

    long long ban_until = 0;
    bool is_permanent = false;
    long long duration_seconds = 0;

    std::string lower_token = duration_token;
    std::transform(lower_token.begin(), lower_token.end(), lower_token.begin(), ::tolower);

    if (lower_token == "perma")
    {
        is_permanent = true;
        ban_until = 9999999999LL;
    }
    else
    {
        try
        {
            duration_seconds = std::stoll(duration_token);
        }
        catch (...)
        {
            hb::console::error("Invalid duration or 'perma' flag. Usage: ban-player <character_name> <duration_seconds|perma> [reason]");
            return;
        }

        ban_until = (duration_seconds > 0) ? (std::time(nullptr) + duration_seconds) : 9999999999LL;
    }

    std::string reason = "";
    std::getline(ss, reason);
    
    size_t first = reason.find_first_not_of(" \t");
    if (first == std::string::npos)
    {
        reason = "Sin motivo especificado";
    }
    else
    {
        size_t last = reason.find_last_not_of(" \t");
        reason = reason.substr(first, (last - first + 1));
    }

    std::string account_name = "";
    int target_client = -1;

    for (int i = 1; i < hb::server::config::MaxClients; i++)
    {
        if (game->m_client_list[i] != nullptr && game->m_client_list[i]->m_is_init_complete)
        {
            if (_stricmp(game->m_client_list[i]->m_char_name, char_name_str.c_str()) == 0)
            {
                target_client = i;
                account_name = game->m_client_list[i]->m_account_name;
                break;
            }
        }
    }

    if (account_name.empty())
    {
        account_name = char_name_str;
    }

    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(account_name.c_str(), &db, dbPath))
    {
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS account_bans (ban_until INTEGER, reason TEXT);", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "DELETE FROM account_bans;", nullptr, nullptr, nullptr);

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "INSERT INTO account_bans (ban_until, reason) VALUES (?, ?);", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int64(stmt, 1, ban_until);
            sqlite3_bind_text(stmt, 2, reason.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        sqlite3_close(db);

        if (is_permanent)
        {
            hb::console::success("Account '{}' banned PERMANENTLY. Reason: {}", account_name, reason);
            hb::logger::log<hb::log_channel::commands>("ban-player: account '{}' banned PERMANENTLY. Reason: {}", account_name, reason);
        }
        else
        {
            hb::console::success("Account '{}' banned for {} seconds. Reason: {}", account_name, duration_seconds, reason);
            hb::logger::log<hb::log_channel::commands>("ban-player: account '{}' banned for {}s. Reason: {}", account_name, duration_seconds, reason);
        }

        if (target_client != -1)
        {
            auto* client = game->m_client_list[target_client];
            if (client->m_socket != nullptr)
            {
                client->m_socket->close_connection();
            }
        }
    }
    else
    {
        hb::console::error("Could not find account database for '{}'.", account_name);
    }
}