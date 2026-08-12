#include "CmdKickPlayer.h"
#include "Game.h"
#include "ServerConfig.h"
#include "ServerConsole.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "Client.h"
#include <cstring>

void CmdKickPlayer::execute(CGame* game, const char* args)
{
    if (args == nullptr || args[0] == '\0')
    {
        hb::console::error("Usage: kick-player <character_name>");
        return;
    }

    char char_name[64] = {};
    std::sscanf(args, "%s", char_name);

    int target_client = -1;
    for (int i = 1; i < hb::server::config::MaxClients; i++)
    {
        if (game->m_client_list[i] != nullptr && game->m_client_list[i]->m_is_init_complete)
        {
            if (_stricmp(game->m_client_list[i]->m_char_name, char_name) == 0)
            {
                target_client = i;
                break;
            }
        }
    }

    if (target_client == -1)
    {
        hb::console::error("Player '{}' not found online.", char_name);
        return;
    }

    auto* client = game->m_client_list[target_client];
    
    if (client->m_socket != nullptr)
    {
        client->m_socket->close_connection();
    }

    hb::console::success("Player '{}' has been kicked.", char_name);
    hb::logger::log<hb::log_channel::commands>("kick-player: kicked character '{}'", char_name);
}