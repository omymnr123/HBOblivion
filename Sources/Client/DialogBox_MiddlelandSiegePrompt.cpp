#include "DialogBox_MiddlelandSiegePrompt.h"
#include "Game.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include "Packet/SharedPackets.h"
#include "PacketSendHelpers.h"

using namespace hb::client::sprite_id;
using namespace hb::shared::net;

DialogBox_MiddlelandSiegePrompt::DialogBox_MiddlelandSiegePrompt(CGame* game)
	: IDialogBox(DialogBoxId::MiddlelandSiegePrompt, game)
{
	set_default_rect(0, 0, 310, 150);
}

void DialogBox_MiddlelandSiegePrompt::on_draw()
{
	short sX = m_x;
	short sY = m_y;

	// Draw Background
	draw_new_dialog_box(InterfaceNdGame4, sX, sY, 2);

	// Draw text
	hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 35, 310, 15, "Middleland Siege is about to start", hb::shared::text::TextStyle::with_shadow(GameColors::UIYellow), hb::shared::text::Align::TopCenter);
	hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 55, 310, 15, "Do you wish to teleport now?", hb::shared::text::TextStyle::with_shadow(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
	
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	// Accept Button
	hb::shared::render::Color color_accept = hb::shared::render::Color(150, 150, 150);
	if (mouse_x >= sX + 50 && mouse_x <= sX + 110 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
		color_accept = hb::shared::render::Color(255, 255, 255);
	hb::shared::text::draw_text(GameFont::Default, sX + 60, sY + 105, "Accept", hb::shared::text::TextStyle::with_shadow(color_accept));

	// Cancel Button
	hb::shared::render::Color color_cancel = hb::shared::render::Color(150, 150, 150);
	if (mouse_x >= sX + 180 && mouse_x <= sX + 240 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
		color_cancel = hb::shared::render::Color(255, 255, 255);
	hb::shared::text::draw_text(GameFont::Default, sX + 190, sY + 105, "Cancel", hb::shared::text::TextStyle::with_shadow(color_cancel));
}

bool DialogBox_MiddlelandSiegePrompt::on_click()
{
	short sX = m_x;
	short sY = m_y;
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	// Click on Accept
	if (mouse_x >= sX + 50 && mouse_x <= sX + 110 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		auto pkt = hb::net::make_common_command(CommonType::ReqAcceptMiddlelandSiegeTeleport, m_game->m_player->m_player_x, m_game->m_player->m_player_y);
		send_game_packet(pkt);
		disable_this_dialog();
		return true;
	}

	// Click on Cancel
	if (mouse_x >= sX + 180 && mouse_x <= sX + 240 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		auto pkt = hb::net::make_common_command(CommonType::ReqCancelMiddlelandSiegeTeleport, m_game->m_player->m_player_x, m_game->m_player->m_player_y);
		send_game_packet(pkt);
		disable_this_dialog();
		return true;
	}

	return false;
}

bool DialogBox_MiddlelandSiegePrompt::on_enable(int type, int64_t v1, int v2, const char* string)
{
	return true;
}

bool DialogBox_MiddlelandSiegePrompt::on_disable()
{
	return true;
}
