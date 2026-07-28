#include "DialogBox_GuildOperation.h"
#include "Game.h"
#include "LAN_ENG.H"
#include "TextLibExt.h"
#include "TextInputManager.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include <format>

using namespace hb::client::sprite_id;

DialogBox_GuildOperation::DialogBox_GuildOperation(CGame* game)
	: IDialogBox(DialogBoxId::GuildOperation, game)
{
	set_default_rect(497, 57, 258, 339);
}

void DialogBox_GuildOperation::on_draw()
{
	short sX = m_x;
	short sY = m_y;

	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);

	hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 80, DRAW_DIALOGBOX_GUILDMENU18, hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

	// Create Button
    hb::shared::render::Color color_create = hb::shared::render::Color(150, 150, 150);
    if (mx >= sX + 50 && mx <= sX + 110 && my >= sY + 160 && my <= sY + 180)
        color_create = hb::shared::render::Color(255, 255, 255);
	hb::shared::text::draw_text(GameFont::Default, sX + 60, sY + 165, "Create", hb::shared::text::TextStyle::with_shadow(color_create));

	// Cancel Button
    hb::shared::render::Color color_cancel = hb::shared::render::Color(150, 150, 150);
    if (mx >= sX + 150 && mx <= sX + 210 && my >= sY + 160 && my <= sY + 180)
        color_cancel = hb::shared::render::Color(255, 255, 255);
	hb::shared::text::draw_text(GameFont::Default, sX + 160, sY + 165, "Cancel", hb::shared::text::TextStyle::with_shadow(color_cancel));

}

bool DialogBox_GuildOperation::on_click()
{
	short sX = m_x;
	short sY = m_y;
    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

	if (mx >= sX + 50 && mx <= sX + 110 && my >= sY + 160 && my <= sY + 180)
	{
		// Create clicked
		if (!m_guild_name.empty()) {
            audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			m_game->send_chat_message(std::format("/guild create {}", m_guild_name).c_str());
			m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::GuildOperation);
		}
		return true;
	}
	else if (mx >= sX + 150 && mx <= sX + 210 && my >= sY + 160 && my <= sY + 180)
	{
		// Cancel clicked
        audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::GuildOperation);
		return true;
	}

	return false;
}

bool DialogBox_GuildOperation::on_enable(int type, int64_t v1, int v2, const char* string)
{
	m_guild_name.clear();
	text_input_manager::get().end_input();
	text_input_manager::get().start_input(m_x + 30, m_y + 110, 20, m_guild_name, false, {});
	return true;
}

bool DialogBox_GuildOperation::on_disable()
{
	text_input_manager::get().end_input();
	return true;
}
