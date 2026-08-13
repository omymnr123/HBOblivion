#include "CmdRestart.h"
#include "ServerConsole.h"
#include "Game.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "TimeUtils.h"
#include <cstdlib>

void CmdRestart::execute(CGame* game, const char* args)
{
	int delay_seconds = 0;
	if (args != nullptr && args[0] != '\0')
	{
		delay_seconds = std::atoi(args);
		if (delay_seconds <= 0)
		{
			hb::console::error("Usage: restart [seconds]");
			return;
		}
	}

	hb::console::info("Initiating graceful RESTART...");
	hb::logger::log<hb::log_channel::commands>("restart: initiated (delay={}s)", delay_seconds);

	// Reset state flags (igual que el shutdown normal)
	game->m_shutdown_save_done = false;
	game->m_shutdown_force_logout_done = false;
	game->m_last_shutdown_notice_time = 0;

	// === EL SECRETO MAGICO ===
	// Código 3 le dice al main loop que haga un Auto-Reboot al terminar
	game->m_shutdown_code = 3;

	if (delay_seconds > 0)
	{
		// Iniciar cuenta atrás
		game->m_shutdown_start_time = GameClock::GetTimeMS();
		if (game->m_shutdown_start_time == 0) game->m_shutdown_start_time = 1;
		game->m_shutdown_delay_ms = static_cast<uint32_t>(delay_seconds) * 1000;
		hb::console::info("Restart scheduled in {} seconds.", delay_seconds);
	}
	else
	{
		// Reinicio inmediato
		game->save_all_players();
		game->m_on_exit_process = true;
		game->m_exit_process_time = GameClock::GetTimeMS();
		hb::console::info("Immediate restart executing...");
	}
}