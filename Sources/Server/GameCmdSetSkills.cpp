#include "GameCmdSetSkills.h"
#include "Game.h"
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

bool GameCmdSetSkills::execute(CGame* game, int client_h, const char* args)
{
    if (game->m_client_list[client_h] == nullptr) return true;

    // Verificamos que el ejecutor sea administrador nivel 1000
    if (game->m_client_list[client_h]->m_admin_level < 1000) {
        game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You do not have sufficient permissions.");
        return true;
    }

    char targetName[21] = {0};

    if (std::sscanf(args, "%20s", targetName) == 1)
    {
        int target_h = game->find_client_by_name(targetName);
        if (target_h != 0 && game->m_client_list[target_h] != nullptr)
        {
            auto p = game->m_client_list[target_h];
            
            // Iteramos por todas las habilidades usando el límite oficial del motor
            for (int i = 0; i < hb::shared::limits::MaxSkillType; i++) 
            {
                // Asignamos el valor máximo (100 para skills generales, o ajusta si alguna tolera otro tope)
                p->m_skill_mastery[i] = 100; 

                // ENVIAMOS EL PAQUETE DE RED AL CLIENTE PARA ACTUALIZAR EN CALIENTE (SIN RELOGEAR)
                game->send_notify_msg(0, target_h, Notify::Skill, i, p->m_skill_mastery[i], 0, 0);
            }

            // Notificaciones de éxito para el administrador y el objetivo
            game->send_notify_msg(0, target_h, Notify::NoticeMsg, 0, 0, 0, "All your skills have been maxed out!");
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Skills of the player updated to max successfully.");
        }
        else
        {
            game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player not found or offline.");
        }
    } 
    else 
    {
        game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Uso: /setskills <nombre_personaje>");
    }
    return true;
}