#include "DialogBox_GuildDisbandConfirm.h"
#include "Game.h"
#include "IInput.h"
#include "AudioManager.h"
#include "GameFonts.h"
#include "TextLibExt.h"

using namespace hb::client::sprite_id;

DialogBox_GuildDisbandConfirm::DialogBox_GuildDisbandConfirm(CGame* game)
	: IDialogBox(DialogBoxId::GuildDisbandConfirm, game)
{
	set_default_rect(285, 200, 270, 105);
}

void DialogBox_GuildDisbandConfirm::on_draw()
{
	short sX = m_x;
	short sY = m_y;

	// Draw transparent background box
	m_game->m_Renderer->draw_rect_filled(sX, sY, m_size_x, m_size_y, hb::shared::render::Color::Black(150));

	put_string(sX + 35, sY + 30, "Do you really want to disband?", GameColors::UIOrange);
	put_string(sX + 36, sY + 30, "Do you really want to disband?", GameColors::UIOrange);

	// Accept button
	if (mouse_in(btn_yes))
		draw_new_dialog_box(InterfaceNdButton, sX + 30, sY + 55, 19);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 30, sY + 55, 18);

	// Cancel button
	if (mouse_in(btn_no))
		draw_new_dialog_box(InterfaceNdButton, sX + 170, sY + 55, 3);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 170, sY + 55, 2);
}

bool DialogBox_GuildDisbandConfirm::on_click()
{
	short sX = m_x;
	short sY = m_y;

	// Accept button
	if (mouse_in(btn_yes))
	{
		disable_this_dialog();
		disable_dialog_box(DialogBoxId::Guild);
		m_game->send_chat_message("/guild disband");
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	// Cancel button
	if (mouse_in(btn_no))
	{
		disable_this_dialog();
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	return false;
}
