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
            
            // 1. Cambiamos el nivel
            p->m_level = newLevel;
            
            // 2. Le damos la experiencia base exacta de ese nivel para que no de "Connection Lost"
            p->m_exp = game->get_level_exp(newLevel);
            
            // 3. Calculamos y le damos los puntos de estadisticas (fuerza, destreza...) que se merece
            int stats = p->m_str + p->m_dex + p->m_vit + p->m_int + p->m_mag + p->m_charisma;
            p->m_levelup_pool = (p->m_level - 1) * game->m_levelup_stat_gain - (stats - game->m_base_stat_total);
            if (p->m_levelup_pool < 0) p->m_levelup_pool = 0;
            
            // 4. ¡Avisamos a la pantalla del jugador en tiempo real para que cambie los numeros!
            game->send_notify_msg(0, target_h, Notify::SuperAttackLeft, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::LevelUp, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::Exp, 0, 0, 0, 0);
            game->send_notify_msg(0, target_h, Notify::LevelUpPoints, 0, 0, 0, 0);

            // Te avisamos a ti (el GM) de que ha funcionado
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Nivel y EXP cambiados con exito.");
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