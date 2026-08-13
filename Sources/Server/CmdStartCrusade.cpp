#include "CmdStartCrusade.h"
#include "ServerConsole.h"
#include "Game.h"
#include "WarManager.h"
#include "Log.h"
#include "ServerLogChannels.h"

void CmdStartCrusade::execute(CGame* game, const char* args)
{
	if (game->m_is_crusade_mode)
	{
		hb::console::error("La Crusade ya esta activa.");
		return;
	}

	// Llama al gestor moderno para iniciar la guerra
	game->m_war_manager->global_start_crusade_mode();
	
	hb::console::success("Crusade iniciada manualmente desde la consola.");
	hb::logger::log<hb::log_channel::commands>("startcrusade: ejecutado manualmente desde la consola");
}