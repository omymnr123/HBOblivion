#include "CmdShutdown.h"
#include "ServerConsole.h"
#include "Game.h"
#include "Log.h"
#include "ServerLogChannels.h"
#include "NetMessages.h"
#include "TimeUtils.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace hb::shared::net;
using namespace hb::server::config;

void CmdShutdown::execute(CGame* game, const char* args)
{
	int delay_seconds = 0;

	// Parse: shutdown [seconds]
	if (args != nullptr && args[0] != '\0')
	{
		char* end = nullptr;
		long val = std::strtol(args, &end, 10);
		if (end != args && val >= 0)
		{
			delay_seconds = static_cast<int>(val);
		}
		else
		{
			hb::console::error("Usage: shutdown [seconds]");
			return;
		}
	}

	hb::console::info("Initiating graceful shutdown...");
	hb::logger::log<hb::log_channel::commands>("shutdown: initiated (delay={}s)", delay_seconds);

	// Reset state flags
	game->m_shutdown_save_done = false;
	game->m_shutdown_force_logout_done = false;
	game->m_last_shutdown_notice_time = 0;

	if (delay_seconds > 0)
	{
		// Schedule delayed shutdown
		game->m_shutdown_start_time = GameClock::GetTimeMS();
		if (game->m_shutdown_start_time == 0) game->m_shutdown_start_time = 1; // Ensure it's not 0
		game->m_shutdown_delay_ms = static_cast<uint32_t>(delay_seconds) * 1000;
		hb::console::info("Shutdown scheduled in {} seconds.", delay_seconds);
	}
	else
	{
		// Immediate shutdown
		// Save all players immediately as a safety snapshot
		int count = game->save_all_players();
		hb::console::success("Saved {} player(s)", count);

		game->m_on_exit_process = true;
		game->m_exit_process_time = GameClock::GetTimeMS();
		hb::console::info("Disconnecting players and shutting down...");
	}
}
