#include "DialogBox_NpcCreator.h"
#include "Game.h"
#include "GlobalDef.h"
#include "SpriteID.h"
#include "NetMessages.h"
#include "PacketSendHelpers.h"

#include "GameFonts.h"
#include "TextLibExt.h"
#include "TextInputManager.h"
#include "TextFieldRenderer.h"
#include <algorithm>
#include <format>
#include <cstring>
#include "IInput.h"
#include "AudioManager.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
using render_color = hb::shared::render::Color;

// Layout constants
namespace layout
{
	// Shared
	constexpr int pad = 12;
	constexpr int content_x1 = 12;
	constexpr int content_x2 = 246;
	constexpr int content_w = content_x2 - content_x1;

	// Search page
	constexpr int search_bar_y = 30;
	constexpr int search_bar_h = 20;
	constexpr int list_y = 56;
	constexpr int row_h = 18;
	constexpr int list_rows = 11;
	constexpr int list_h = list_rows * row_h;
	constexpr int status_y = list_y + list_h + 2;

	// Actions
	constexpr int spawn_area_y = status_y + 20;
	constexpr int count_sel_y = spawn_area_y;
	constexpr int count_sel_w = 70;
	constexpr int btn_y = spawn_area_y + 24;
	constexpr int btn_w = 100;
}

namespace dd_style
{
	const auto bg           = render_color(40, 35, 28, 190);
	const auto border       = render_color(80, 70, 50);
	const auto border_hover = render_color(140, 125, 90);
	const auto border_open  = render_color(180, 160, 100);
	const auto list_bg      = render_color(30, 25, 18, 235);
	const auto list_border  = render_color(100, 90, 60);
	const auto item_hover   = render_color(90, 75, 45, 180);
	const auto scrollbar    = render_color(130, 115, 75, 160);
}

DialogBox_NpcCreator::DialogBox_NpcCreator(CGame* game)
	: IDialogBox(DialogBoxId::NpcCreator, game)
{
	set_default_rect(0, 0, 258, 339);
	m_can_close_on_right_click = true;
}

bool DialogBox_NpcCreator::on_disable()
{
	text_input_manager::get().end_input();
	m_initial_load = false;
	m_last_sent_search.clear();
	m_count_dropdown_open = false;
	return true;
}

bool DialogBox_NpcCreator::on_enable(int type, int64_t v1, int v2, const char* string)
{
	m_search_text.clear();
	m_last_sent_search.clear();
	m_result_count = 0;
	m_selected_index = -1;
	m_scroll_offset = 0;
	m_initial_load = false;
	m_item_count = 1;
	m_count_dropdown_open = false;
	return true;
}

void DialogBox_NpcCreator::receive_search_results(const hb::net::PacketNotifyGameMasterNpcSearchResult* pkt)
{
	m_result_count = std::min(static_cast<int>(pkt->count), 200);
	std::memcpy(m_results, pkt->entries, m_result_count * sizeof(hb::net::GameMasterNpcSearchEntry));

	// Maintain selection if possible
	if (m_selected_index >= m_result_count)
		m_selected_index = -1;

	// Clamp scroll
	int max_scroll = std::max(0, m_result_count - layout::list_rows);
	m_scroll_offset = std::clamp(m_scroll_offset, 0, max_scroll);
}

void DialogBox_NpcCreator::on_draw()
{
	auto& text_mgr = text_input_manager::get();
	short sX = m_x;
	short sY = m_y;
	short size_x = m_size_x;
	short size_y = m_size_y;
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());
	short z = static_cast<short>(hb::shared::input::get_mouse_wheel_delta());

	// Background
	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0, size_x, size_y);
	

	// Title
	hb::shared::text::draw_text_aligned(GameFont::Bitmap1,
		sX, sY + 8, size_x, 15,
		"NPC Spawner",
		hb::shared::text::TextStyle::with_integrated_shadow(GameColors::UIWarningRed),
		hb::shared::text::Align::TopCenter);

	// Text input
	if (!text_mgr.is_active())
	{
		text_input_manager::get().start_input(sX + 70, sY + layout::search_bar_y + 5, 20, m_search_text);
		m_last_sx = sX;
		m_last_sy = sY;
	}
	else if (sX != m_last_sx || sY != m_last_sy)
	{
		text_mgr.end_input();
		text_input_manager::get().start_input(sX + 70, sY + layout::search_bar_y + 5, 20, m_search_text);
		m_last_sx = sX;
		m_last_sy = sY;
	}

	put_string(sX + 16, sY + layout::search_bar_y + 5, "Search:", GameColors::UIWhite);

	// Live search
	if (!m_initial_load || m_search_text != m_last_sent_search)
	{
		m_initial_load = true;
		m_last_sent_search = m_search_text;
		m_scroll_offset = 0;
		m_selected_index = -1;
		{
			auto pkt = hb::net::make_common_command_str(CommonType::GameMasterNpcSearch, player().m_player_x, player().m_player_y);
			std::snprintf(pkt.text, sizeof(pkt.text), "%s", m_search_text.empty() ? "" : m_search_text.c_str());
			m_game->send_game_packet(pkt);
		}
	}

	// Mouse wheel
	if (m_game->get_dialog_box_manager().get_top_id() == DialogBoxId::NpcCreator && z != 0)
	{
		if (m_count_dropdown_open)
		{
			m_dropdown_scroll -= z / 60;
			m_dropdown_scroll = std::clamp(m_dropdown_scroll, 0, std::max(0, 100 - 8));
		}
		else
		{
			m_scroll_offset -= z / 60;
			int max_scroll = std::max(0, m_result_count - layout::list_rows);
			m_scroll_offset = std::clamp(m_scroll_offset, 0, max_scroll);
		}
	}

	// Results list
	for (int i = 0; i < layout::list_rows && (i + m_scroll_offset) < m_result_count; i++)
	{
		int idx = i + m_scroll_offset;
		auto& entry = m_results[idx];
		int ry = sY + layout::list_y + i * layout::row_h;

		bool hover = !m_count_dropdown_open && (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= ry && mouse_y <= ry + layout::row_h - 2);
		bool selected = (idx == m_selected_index);

		if (selected)
		{
			m_game->m_Renderer->draw_rect_filled(
				sX + layout::content_x1, ry, layout::content_w, layout::row_h,
				dd_style::item_hover);
		}

		auto color = (hover || selected) ? GameColors::UIWhite : GameColors::UIMagicBlue;
		hb::shared::text::draw_text_aligned(GameFont::Default,
			sX + layout::content_x1 + 6, ry, layout::content_w - 12, 15,
			entry.name,
			hb::shared::text::TextStyle::from_color(color),
			hb::shared::text::Align::TopLeft);
	}

	// Status line
	if (m_result_count > 0)
	{
		int sy = sY + layout::status_y;
		auto count_str = std::format("{} found", m_result_count);
		put_string(sX + layout::content_x1 + 4, sy, count_str.c_str(), GameColors::UIBlack);

		if (m_result_count > layout::list_rows)
		{
			int max_scroll = m_result_count - layout::list_rows;
			auto scroll_str = std::format("[{}/{}]", m_scroll_offset + 1, max_scroll + 1);
			put_string(sX + 100, sy, scroll_str.c_str(), GameColors::UIBlack);
		}
	}

	// Spawn Controls
	int lx = sX + layout::content_x1;
	int count_x = lx;
	int btn_x = sX + size_x / 2 - layout::btn_w / 2;

	put_string(count_x + 4, sY + layout::count_sel_y - 12, "Amount:", GameColors::UIWhite);
	
	auto count_str = std::to_string(m_item_count);
	bool cnt_hover = !m_count_dropdown_open && (mouse_x >= count_x && mouse_x <= count_x + layout::count_sel_w
		&& mouse_y >= sY + layout::count_sel_y && mouse_y < sY + layout::count_sel_y + 14);
	draw_dropdown_field(count_x, sY + layout::count_sel_y, layout::count_sel_w, count_str.c_str(), m_count_dropdown_open, cnt_hover);

	// Spawn Button
	bool can_spawn = (m_selected_index >= 0 && m_selected_index < m_result_count);
	bool btn_hover = !m_count_dropdown_open && can_spawn && (mouse_x >= btn_x && mouse_x <= btn_x + layout::btn_w
		&& mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18);
	
	auto btn_color = btn_hover ? GameColors::UIWhite : (can_spawn ? GameColors::UIPaleYellow : GameColors::UIDisabledMed);
	hb::shared::text::draw_text_aligned(GameFont::Default,
		btn_x, sY + layout::btn_y, layout::btn_w, 18,
		"SPAWN",
		hb::shared::text::TextStyle::from_color(btn_color));
	if (btn_hover || !can_spawn)
	{
		m_game->m_Renderer->draw_rect_outline(btn_x, sY + layout::btn_y, layout::btn_w, 18, dd_style::border_hover);
	}
	else
	{
		m_game->m_Renderer->draw_rect_outline(btn_x, sY + layout::btn_y, layout::btn_w, 18, dd_style::border);
	}


	// Draw dropdown popup OVER everything
	if (m_count_dropdown_open)
	{
		int max_vis = 8;
		int list_h = max_vis * 14;
		int px = count_x;
		int py = sY + layout::count_sel_y + 14;

		m_game->m_Renderer->draw_rect_filled(px, py, layout::count_sel_w, list_h, dd_style::list_bg);
		m_game->m_Renderer->draw_rect_outline(px, py, layout::count_sel_w, list_h, dd_style::list_border);

		for (int i = 0; i < max_vis; i++)
		{
			int val = i + 1 + m_dropdown_scroll;
			if (val > 100) break;

			int item_y = py + i * 14;
			bool hov = (mouse_x >= px && mouse_x < px + layout::count_sel_w && mouse_y >= item_y && mouse_y < item_y + 14);

			if (hov) m_game->m_Renderer->draw_rect_filled(px + 1, item_y + 1, layout::count_sel_w - 2, 12, dd_style::item_hover);

			auto str = std::to_string(val);
			put_aligned_string(px + 4, px + layout::count_sel_w - 4, item_y + 1, str.c_str(), hov ? GameColors::UIWhite : GameColors::UIDisabledMed);
		}
	}

	// Close button
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_NpcCreator::draw_dropdown_field(int x, int y, int w, const char* text, bool is_open, bool is_hover)
{
	m_game->m_Renderer->draw_rect_filled(x, y, w, 14, dd_style::bg);
	auto bcol = is_open ? dd_style::border_open : (is_hover ? dd_style::border_hover : dd_style::border);
	m_game->m_Renderer->draw_rect_outline(x, y, w, 14, bcol);

	put_aligned_string(x + 4, x + w - 16, y + 1, text, is_hover ? GameColors::UIWhite : GameColors::UIDisabledMed);

	// Chevron
	int cx = x + w - 12;
	int cy = y + 5;
	if (is_open)
	{
		m_game->m_Renderer->draw_line(cx, cy + 3, cx + 3, cy, bcol);
		m_game->m_Renderer->draw_line(cx + 3, cy, cx + 6, cy + 3, bcol);
	}
	else
	{
		m_game->m_Renderer->draw_line(cx, cy, cx + 3, cy + 3, bcol);
		m_game->m_Renderer->draw_line(cx + 3, cy + 3, cx + 6, cy, bcol);
	}
}

bool DialogBox_NpcCreator::on_click()
{
	short sX = m_x;
	short sY = m_y;
	short size_x = m_size_x;
	short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
	short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		m_game->get_dialog_box_manager().disable(DialogBoxId::NpcCreator);
		return true;
	}

	if (m_count_dropdown_open)
	{
		int px = sX + layout::content_x1;
		int py = sY + layout::count_sel_y + 14;
		int list_h = 8 * 14;

		if (mouse_x >= px && mouse_x < px + layout::count_sel_w && mouse_y >= py && mouse_y < py + list_h)
		{
			int sel = (mouse_y - py) / 14;
			int val = sel + 1 + m_dropdown_scroll;
			if (val <= 100)
			{
				m_item_count = val;
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			}
		}
		m_count_dropdown_open = false;
		return true;
	}

	// Click on count dropdown
	int count_x = sX + layout::content_x1;
	if (mouse_x >= count_x && mouse_x <= count_x + layout::count_sel_w
		&& mouse_y >= sY + layout::count_sel_y && mouse_y < sY + layout::count_sel_y + 14)
	{
		m_count_dropdown_open = true;
		m_dropdown_scroll = std::clamp(m_item_count - 4, 0, std::max(0, 100 - 8));
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	// Click on list item
	for (int i = 0; i < layout::list_rows && (i + m_scroll_offset) < m_result_count; i++)
	{
		int ry = sY + layout::list_y + i * layout::row_h;
		if (mouse_x >= sX + layout::content_x1 && mouse_x <= sX + layout::content_x2
			&& mouse_y >= ry && mouse_y <= ry + layout::row_h - 2)
		{
			m_selected_index = i + m_scroll_offset;
			audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			return true;
		}
	}

	// Click on Spawn button
	int btn_x = sX + size_x / 2 - layout::btn_w / 2;
	if (m_selected_index >= 0 && m_selected_index < m_result_count &&
		mouse_x >= btn_x && mouse_x <= btn_x + layout::btn_w &&
		mouse_y >= sY + layout::btn_y && mouse_y <= sY + layout::btn_y + 18)
	{
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		auto cmd = std::format("/spawn {} {}", m_results[m_selected_index].npc_id, m_item_count);
		m_game->send_chat_message(cmd.c_str());
		
		m_game->get_dialog_box_manager().disable(DialogBoxId::NpcCreator);
		return true;
	}

	return false;
}

void DialogBox_NpcCreator::on_enter_pressed()
{
	text_input_manager::get().end_input();
	text_input_manager::get().start_input(m_x + 70, m_y + layout::search_bar_y + 5, 20, m_search_text);
}
