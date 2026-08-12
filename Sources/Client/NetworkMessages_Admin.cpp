#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "DialogBoxIDs.h"
#include "DialogBox_Noticement.h"
#include "AudioManager.h"
#include <cstdio>
#include <cstring>
#include <string>

namespace NetworkMessageHandlers {

void HandleCrashHandler(CGame* game, char* data)
{
	// 0x0BEF: Crash or closes the client? (Calls SE entry !)
	// I'm not sure at all of this function's result, so let's quit game...
	// empty handler - just acknowledge the message
}

void HandleIpAccountInfo(CGame* game, char* data)
{
	std::string temp;
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyIpAccountInfo>(
		data, sizeof(hb::net::PacketNotifyIpAccountInfo));
	if (!pkt) return;
	temp = pkt->text;
	game->add_event_list(temp.c_str());
}

void HandleRewardGold(CGame* game, char* data)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyRewardGold>(
		data, sizeof(hb::net::PacketNotifyRewardGold));
	if (!pkt) return;
	game->m_player->m_reward_gold = pkt->gold;
}

void HandleServerShutdown(CGame* game, char* data)
{
	const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyServerShutdown>(
		data, sizeof(hb::net::PacketNotifyServerShutdown));
	if (!pkt) return;

	// Format and show the top message (same style as config reload)
	std::string notice = std::format("The server will shut down in {} seconds.", pkt->seconds);
	game->set_top_msg(notice.c_str(), 5);

	audio_manager::get().play_game_sound(sound_type::effect, 27, 0);
}

} // namespace NetworkMessageHandlers
