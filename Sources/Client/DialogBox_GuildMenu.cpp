#include "DialogBox_GuildMenu.h"
#include "Game.h"
#include "LAN_ENG.H"
#include "TextLibExt.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include <format>

using namespace hb::client::sprite_id;

DialogBox_GuildMenu::DialogBox_GuildMenu(CGame* game)
	: IDialogBox(DialogBoxId::GuildMenu, game)
{
	set_default_rect(497, 57, 258, 339);
	m_can_close_on_right_click = true;
}

void DialogBox_GuildMenu::on_draw()
{
	short sX = m_x;
	short sY = m_y;

	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
	// m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 19); // Commented out to prevent "Quest" overlay

	// Draw custom title
	hb::shared::text::draw_text(GameFont::Default, sX + 100, sY + 15, "Guild Menu", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

    // Option 1: Make a new guild
    hb::shared::render::Color color1 = hb::shared::render::Color(150, 150, 150);
    if (mx >= sX + 30 && mx <= sX + 230 && my >= sY + 80 && my <= sY + 95)
        color1 = hb::shared::render::Color(255, 255, 255);
    hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 80, std::format("{} (30000 Gold, Level 10)", DRAW_DIALOGBOX_GUILDMENU1).c_str(), hb::shared::text::TextStyle::with_shadow(color1));

    // Option 2: Break up your guild
    hb::shared::render::Color color2 = hb::shared::render::Color(150, 150, 150);
    if (mx >= sX + 30 && mx <= sX + 230 && my >= sY + 100 && my <= sY + 115)
        color2 = hb::shared::render::Color(255, 255, 255);
    hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 100, DRAW_DIALOGBOX_GUILDMENU4, hb::shared::text::TextStyle::with_shadow(color2));

    hb::shared::text::draw_text(GameFont::Default, sX + 40, sY + 200, DRAW_DIALOGBOX_GUILDMENU17, hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
}

bool DialogBox_GuildMenu::on_click()
{
	short sX = m_x;
	short sY = m_y;
    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

    // Click on Option 1 (Make new guild)
    if (mx >= sX + 30 && mx <= sX + 230 && my >= sY + 80 && my <= sY + 95)
    {
        audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
        m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::GuildMenu);
        m_game->get_dialog_box_manager().enable_dialog_box(DialogBoxId::GuildOperation, 0, 0, 0);
        return true;
    }

    // Click on Option 2 (Break up guild)
    if (mx >= sX + 30 && mx <= sX + 230 && my >= sY + 100 && my <= sY + 115)
    {
        audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
        m_game->send_chat_message("/guild disband");
        m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::GuildMenu);
        return true;
    }

	return false;
}

bool DialogBox_GuildMenu::on_enable(int type, int64_t v1, int v2, const char* string)
{
	return true;
}

bool DialogBox_GuildMenu::on_disable()
{
	return true;
}
