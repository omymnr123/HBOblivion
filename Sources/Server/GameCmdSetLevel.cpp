#include "GameCmdSetLevel.h"
#include "Game.h"
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

bool GameCmdSetLevel::execute(CGame* game, int client_h, const char* args)
{
    if (game->m_client_list[client_h] == nullptr) return true;

    char targetName[21] = {0};
    int newLevel = 1;

    if (std::sscanf(args, "%20s %d", targetName, &newLevel) == 2)
    {
        int target_h = game->find_client_by_name(targetName);
        if (target_h != 0 && game->m_client_list[target_h] != nullptr)
        {
            auto p = game->m_client_list[target_h];
            
            // Limitamos el nivel al m_max_level del servidor para evitar desconexiones por desbordamiento
            if (newLevel > game->m_max_level) {
                newLevel = game->m_max_level;
            }
            
            p->m_level = newLevel;
            p->m_exp = game->get_level_exp(newLevel);
            
            int stats = p->m_str + p->m_dex + p->m_vit + p->m_int + p->m_mag + p->m_charisma;
            
            // Sumamos el + p->m_prestige_bonus_stats al final de la ecuación:
            p->m_levelup_pool = (p->m_level - 1) * game->m_levelup_stat_gain - (stats - game->m_base_stat_total) + p->m_prestige_bonus_stats;
            
            if (p->m_levelup_pool < 0) p->m_levelup_pool = 0;
            
            game->send_notify_msg(0, target_h, Notify::SuperAttackLeft, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::LevelUp, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::Exp, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::LevelUpPoints, 0, 0, 0, 0);

            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Nivel cambiado al maximo permitido sin errores.");
        }
        else
        {
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Jugador no encontrado u offline.");
        }
    } else {
        game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Uso: /setlevel <personaje> <nivel>");
    }
    return true;
}