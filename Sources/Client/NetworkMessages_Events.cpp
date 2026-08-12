#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include <cstdio>
#include <cstring>
#include "DialogBoxIDs.h"
#include <format>
#include <string>

namespace NetworkMessageHandlers {
	void HandleSpawnEvent(CGame* game, char* data)
	{
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifySpawnEvent>(
			data, sizeof(hb::net::PacketNotifySpawnEvent));
		if (!pkt) return;

		// Sarcofago Map Markers
		if (pkt->monster_id >= 150 && pkt->monster_id <= 152) {
			game->m_sarcofago_id = pkt->monster_id;
			game->m_sarcofago_x = pkt->x;
			game->m_sarcofago_y = pkt->y;
			return; // Do not trigger the normal 30-second monster event
		} else if (pkt->monster_id == 0) {
			game->m_sarcofago_id = 0;
			game->m_sarcofago_x = 0;
			game->m_sarcofago_y = 0;
			return;
		}

		game->m_monster_id = pkt->monster_id;
		game->m_event_x = pkt->x;
		game->m_event_y = pkt->y;
		game->m_monster_event_time = game->m_cur_time;
	}

	void HandleMonsterCount(CGame* game, char* data)
	{
		std::string txt;
		int count;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyMonsterCount>(
			data, sizeof(hb::net::PacketNotifyMonsterCount));
		if (!pkt) return;
		count = pkt->count;
		txt = std::format("Rest Monster :{}", count);
		game->add_event_list(txt.c_str(), 10);
	}

	void HandleResurrectPlayer(CGame* game, char* data)
	{
		game->get_dialog_box_manager().enable_dialog_box(DialogBoxId::Resurrect, 0, 0, 0);
	}
} // namespace NetworkMessageHandlers
