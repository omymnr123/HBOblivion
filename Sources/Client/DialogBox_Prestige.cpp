#include "DialogBox_Prestige.h"
#include "Game.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "IInput.h"
#include "Packet/SharedPackets.h"
#include "PacketSendHelpers.h"
#include "Screen_OnGame.h"
#include "AudioManager.h"

using namespace hb::shared::net;
using namespace hb::client::net;
using namespace hb::client::sprite_id;

DialogBox_Prestige::DialogBox_Prestige(CGame* game)
	: IDialogBox(DialogBoxId::Prestige, game)
{
	// Tamaño estándar para ventana de diálogo con botones (igual que CityHall)
	set_default_rect(497, 57, 258, 339); 
}

void DialogBox_Prestige::on_draw()
{
	short sX = m_x;
	short sY = m_y;
	short size_x = m_size_x;

	// Dibujar fondo de ventana negro y marco decorativo (estándar de Helbreath)
	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
	m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 18);

	// Dibujar los textos principales
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 60, (sX + size_x) - sX, 15, "--- THE PRESTIGE ---", hb::shared::text::TextStyle::from_color(GameColors::UIYellow), hb::shared::text::Align::TopCenter);
    
    // Mostrar Prestigio Actual y Siguiente Prestigio dinámicamente
    int current_prestige = player().m_prestige_level;
    int target_prestige = current_prestige + 1;
    std::string prestige_info_text = std::format("Current Prestige: {}  ->  Next: {}", current_prestige, target_prestige);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 85, (sX + size_x) - sX, 15, prestige_info_text.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);

    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 105, (sX + size_x) - sX, 15, "Are you ready to transcend?", hb::shared::text::TextStyle::from_color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
    
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 130, (sX + size_x) - sX, 15, "Warning: Your Level will be reset to 1.", hb::shared::text::TextStyle::from_color(GameColors::UIRed), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 145, (sX + size_x) - sX, 15, "Leveling up will become 15% harder.", hb::shared::text::TextStyle::from_color(GameColors::UIRed), hb::shared::text::Align::TopCenter);
    
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 175, (sX + size_x) - sX, 15, "In return, you will gain +3 Stat Points", hb::shared::text::TextStyle::from_color(GameColors::UIGreen), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 190, (sX + size_x) - sX, 15, "& 10HP & 10MP.", hb::shared::text::TextStyle::from_color(GameColors::UIGreen), hb::shared::text::Align::TopCenter);
	hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 205, (sX + size_x) - sX, 15, "permanently to distribute as you wish.", hb::shared::text::TextStyle::from_color(GameColors::UIGreen), hb::shared::text::Align::TopCenter);

    // Mostrar requisitos
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 220, (sX + size_x) - sX, 15, "Requirements:", hb::shared::text::TextStyle::from_color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 235, (sX + size_x) - sX, 15, "- Reach Level 180", hb::shared::text::TextStyle::from_color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 250, (sX + size_x) - sX, 15, "- Weight under 50", hb::shared::text::TextStyle::from_color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

    // Cálculo dinámico del oro requerido para el siguiente nivel de prestigio
    uint64_t required_gold = static_cast<uint64_t>(target_prestige) * 250000ULL;
    std::string gold_req_text = std::format("- Gold: {}", required_gold);
    hb::shared::text::draw_text_aligned(GameFont::Default, sX, sY + 265, (sX + size_x) - sX, 15, gold_req_text.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);

	// Dibujar los Botones de "Aceptar" y "Cancelar"
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	// Botón Izquierdo (YES)
	if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
	else
		m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

	// Botón Derecho (NO)
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
	else
		m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
}

bool DialogBox_Prestige::on_click()
{
	short sX = m_x;
	short sY = m_y;
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	// Clic en el botón YES
	if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		// Enviamos la petición secreta de Prestige al Servidor (El código 0x0F16 que creamos)
		auto pkt = hb::net::make_common_command(CommonType::ReqPrestige, m_game->m_player->m_player_x, m_game->m_player->m_player_y);
		m_game->send_game_packet(pkt);

		// Cerramos la ventana
		m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::Prestige);
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	// Clic en el botón NO
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		// Simplemente cerramos la ventana
		m_game->get_dialog_box_manager().disable_dialog_box(DialogBoxId::Prestige);
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	return false;
}