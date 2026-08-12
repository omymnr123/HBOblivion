#include "DialogBox_GuildInvite.h"
#include "Game.h"
#include "TextLibExt.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include <format>
#include <cstring>

using namespace hb::client::sprite_id;

DialogBox_GuildInvite::DialogBox_GuildInvite(CGame* game)
	: IDialogBox(DialogBoxId::GuildInvite, game)
{
	set_default_rect(0, 0, 310, 150);
}

void DialogBox_GuildInvite::on_draw()
{
	short sX = m_x;
	short sY = m_y;

	// Draw Background
	draw_new_dialog_box(InterfaceNdGame4, sX, sY, 2);

	// Draw text
	hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 35, std::format("{} has invited you", m_inviter_name).c_str(), hb::shared::text::TextStyle::with_shadow(GameColors::UIYellow));
	hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 55, std::format("to join {}.", m_guild_name).c_str(), hb::shared::text::TextStyle::with_shadow(GameColors::UIYellow));
	
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

bool DialogBox_GuildInvite::on_click()
{
	short sX = m_x;
	short sY = m_y;
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	// Click on Accept
	if (mouse_x >= sX + 50 && mouse_x <= sX + 110 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		m_game->send_chat_message("/guild accept");
		disable_this_dialog();
		return true;
	}

	// Click on Cancel
	if (mouse_x >= sX + 180 && mouse_x <= sX + 240 && mouse_y >= sY + 100 && mouse_y <= sY + 120)
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		disable_this_dialog();
		return true;
	}

	return false;
}

bool DialogBox_GuildInvite::on_enable(int type, int64_t v1, int v2, const char* string)
{
	if (string)
	{
		std::string str(string);
		size_t pos = str.find(':');
		if (pos != std::string::npos)
		{
			m_inviter_name = str.substr(0, pos);
			m_guild_name = str.substr(pos + 1);
		}
		else
		{
			m_inviter_name = "Someone";
			m_guild_name = "a guild";
		}
	}
	return true;
}

bool DialogBox_GuildInvite::on_disable()
{
	return true;
}
