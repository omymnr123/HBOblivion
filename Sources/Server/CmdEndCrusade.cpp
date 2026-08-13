#include "CmdEndCrusade.h"
#include "ServerConsole.h"
#include "Game.h"
#include "WarManager.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include <cstdlib>

void CmdEndCrusade::execute(CGame* game, const char* args)
{
	if (!game->m_is_crusade_mode)
	{
		hb::console::error("No hay ninguna Crusade activa para finalizar.");
		return;
	}

	int winner_side = 0; // Por defecto es empate (0)

	// Si se pasa un argumento, intentar usarlo como el bando ganador
	if (args != nullptr && args[0] != '\0')
	{
		winner_side = std::atoi(args);
		if (winner_side < 0 || winner_side > 2)
		{
			hb::console::error("Bando ganador invalido. Usa 0=Empate, 1=Aresden, 2=Elvine.");
			return;
		}
	}

	// Termina la guerra y asigna al ganador
	game->m_war_manager->manual_end_crusade_mode(winner_side);
	
	hb::console::success("Crusade finalizada manualmente. Bando ganador: {}", winner_side);
	hb::logger::log<hb::log_channel::commands>("endcrusade: finalizada manualmente, ganador: {}", winner_side);
}