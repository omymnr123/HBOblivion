#include "GameCmdRevive.h"
#include "Game.h"
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

bool GameCmdRevive::execute(CGame* game, int client_h, const char* args)
{
    if (game->m_client_list[client_h] == nullptr) return true;

    char targetName[21] = {0};
    if (std::sscanf(args, "%20s", targetName) == 1)
    {
        int target_h = game->find_client_by_name(targetName);
        if (target_h != 0 && game->m_client_list[target_h] != nullptr)
        {
            // Le damos 1 de vida y 1 de mana para que despierte
            game->m_client_list[target_h]->m_hp = 1;
            game->m_client_list[target_h]->m_mp = 1;
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Jugador revivido.");
        }
    } else {
        game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Uso: /revive <personaje>");
    }
    return true;
}