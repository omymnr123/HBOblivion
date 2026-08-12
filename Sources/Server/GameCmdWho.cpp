#include "GameCmdWho.h"
#include "Game.h"
#include <cstdio>
#include <cstring>

using namespace hb::shared::net;

bool GameCmdWho::execute(CGame* game, int client_h, const char* args)
{
    // Verificamos que el jugador que ejecuta el comando es válido
    if (game->m_client_list[client_h] == nullptr) return true;

    int aresden_count = 0;
    int elvine_count = 0;
    int neutral_count = 0;

    // Bucle moderno de C++: recorre la lista completa de jugadores automáticamente.
    // Así evitamos tener que usar "DEF_MAXCLIENTS".
    for (auto client : game->m_client_list)
    {
        // Solo contamos a los jugadores que existan y hayan terminado de loguear
        if (client != nullptr && client->m_is_init_complete)
        {
            if (client->m_side == 1) { // 1 = Aresden
                aresden_count++;
            }
            else if (client->m_side == 2) { // 2 = Elvine
                elvine_count++;
            }
            else { // 0 = Neutral u otros
                neutral_count++;
            }
        }
    }

    int total = aresden_count + elvine_count + neutral_count;
    char who_msg[128] = {0};

    // Formateamos el texto tal como lo pediste
    std::snprintf(who_msg, sizeof(who_msg), "Aresden: %d, Elvine: %d, Neutral: %d, Total: %d.", 
                  aresden_count, elvine_count, neutral_count, total);

    // Enviamos el mensaje al jugador que escribió el comando
    game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, who_msg);

    return true;
}