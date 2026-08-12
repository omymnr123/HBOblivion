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

	// Dibujamos el fondo del diálogo de creación de guild
	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);

	// Texto de instrucción superior
	hb::shared::text::draw_text(GameFont::Default, sX + 30, sY + 80, DRAW_DIALOGBOX_GUILDMENU18, hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

	// Coordenadas estándar para los botones
	int btn1_x = sX + 35;
	int btn1_y = sY + 160;
	int btn2_x = sX + 145;
	int btn2_y = sY + 160;

	// Botón 1: Accept (Gráfico puro, sin texto superpuesto)
	// Frames 8 y 9 suelen ser Accept en HB. Si en tu cliente sale otro botón, prueba 4/5 o 12/13.
	bool btn1_hover = (mx >= btn1_x && mx <= btn1_x + 95 && my >= btn1_y && my <= btn1_y + 23);
	m_game->draw_new_dialog_box(InterfaceNdButton, btn1_x, btn1_y, btn1_hover ? 19 : 18);

	// Botón 2: Decline (Gráfico puro, sin texto superpuesto)
	// Frames 10 y 11 suelen ser Decline.
	bool btn2_hover = (mx >= btn2_x && mx <= btn2_x + 95 && my >= btn2_y && my <= btn2_y + 23);
	m_game->draw_new_dialog_box(InterfaceNdButton, btn2_x, btn2_y, btn2_hover ? 3 : 2);
}

bool DialogBox_GuildOperation::on_click()
{
	short sX = m_x;
	short sY = m_y;
    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

	int btn1_x = sX + 35;
	int btn1_y = sY + 160;
	int btn2_x = sX + 145;
	int btn2_y = sY + 160;

	// Click en Botón 1 (Accept)
	if (mx >= btn1_x && mx <= btn1_x + 95 && my >= btn1_y && my <= btn1_y + 23)
	{
		if (!m_guild_name.empty()) {
            audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			m_game->send_chat_message(std::format("/guild create {}", m_guild_name).c_str());
			m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::GuildOperation);
		}
		return true;
	}
	// Click en Botón 2 (Decline)
	else if (mx >= btn2_x && mx <= btn2_x + 95 && my >= btn2_y && my <= btn2_y + 23)
	{
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