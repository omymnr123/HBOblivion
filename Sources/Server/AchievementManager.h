#pragma once
#include "ServerConsole.h"

// Clase de logros "Header-Only" (no necesita archivo .cpp ni tocar Visual Studio)
class AchievementManager {
public:
    static AchievementManager& get() {
        static AchievementManager instance;
        return instance;
    }

    // Usamos 'inline' para que el compilador lo integre directamente
    inline void unlock_achievement(void* player, int achievement_id) {
        if (!player) return;

        // Por ahora lanzamos un mensaje verde por la consola del servidor para confirmar que el código compila y enlaza bien.
        // En cuanto veamos este mensaje al poner /giveitem, cambiaremos esto por el código de SQLite.
        hb::console::success("LOGRO ID {} DESBLOQUEADO CORRECTAMENTE!", achievement_id);
    }
};