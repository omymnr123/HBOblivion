#include "GameCmdTeleport.h"
#include "Game.h"
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

bool GameCmdTeleport::execute(CGame* game, int client_h, const char* args)
{
    if (game->m_client_list[client_h] == nullptr) return true;

    char mapName[21] = {0};
    int x = -1;
    int y = -1;

    int parsed = std::sscanf(args, "%20s %d %d", mapName, &x, &y);
    if (parsed >= 1)
    {
        if (game->gm_teleport_to(client_h, mapName, x, y)) {
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Successfully teleported.");
        } else {
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Failed (Invalid Map).");
        }
    } else {
        game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /tp <map> [x] [y]");
    }
    return true;
}