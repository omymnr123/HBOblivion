#include "DialogBox_Guild.h"
#include "Game.h"
#include "LAN_ENG.H"
#include "TextLibExt.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include "ItemTooltip.h"
#include "Render/IRenderer.h"
#include <format>
#include <cstring>
#include <algorithm>

using namespace hb::client::sprite_id;
using namespace hb::shared::net;

DialogBox_Guild::DialogBox_Guild(CGame* game)
	: IDialogBox(DialogBoxId::Guild, game)
{
	set_default_rect(0, 0, 258, 339); // Correct size for InterfaceNdText background
	m_can_close_on_right_click = true;
	std::memset(&m_guild_data, 0, sizeof(m_guild_data));
}

void DialogBox_Guild::update_members(const PacketGuildMemberList& pkt)
{
	m_guild_data = pkt;
	if (m_selected_member >= pkt.member_count)
		m_selected_member = -1;
	sort_members();
}

void DialogBox_Guild::sort_members()
{
	if (m_guild_data.member_count <= 1) return;
	
	std::sort(m_guild_data.members, m_guild_data.members + m_guild_data.member_count, [this](const GuildMemberInfo& a, const GuildMemberInfo& b) {
		if (m_sort_method == 0) { // Name
			return std::strcmp(a.name, b.name) < 0;
		} else if (m_sort_method == 1) { // Rank
			if (a.rank != b.rank) return a.rank < b.rank; // 1 is Master, lower is higher rank
			return std::strcmp(a.name, b.name) < 0;
		} else if (m_sort_method == 2) { // Map / Date
			int cmp = std::strcmp(a.extra_info, b.extra_info);
			if (cmp != 0) return cmp < 0;
			return std::strcmp(a.name, b.name) < 0;
		} else { // Status
			if (a.is_online != b.is_online) return a.is_online > b.is_online; // Online first
			return std::strcmp(a.name, b.name) < 0;
		}
	});
}

int DialogBox_Guild::get_my_guild_rank() const
{
	for (int i = 0; i < m_guild_data.member_count; ++i) {
		if (_stricmp(m_game->m_player->m_player_name.c_str(), m_guild_data.members[i].name) == 0) {
			return m_guild_data.members[i].rank;
		}
	}
	return 3; // Default to member
}

void DialogBox_Guild::on_draw()
{
	// Periodic real-time update every 3 seconds
	if (m_game->m_time - m_last_update_time > 3000) {
		m_last_update_time = m_game->m_time;
		
		hb::net::packet_base pkt;
		std::memset(&pkt, 0, sizeof(pkt));
		auto* header = reinterpret_cast<hb::net::PacketHeader*>(&pkt);
		header->msg_id = MsgId::GuildSystem;
		header->msg_type = GuildSystemType::RequestMembers;
		send_game_packet_impl(pkt, sizeof(hb::net::PacketHeader), true);
	}

	short sX = m_x;
	short sY = m_y;

	// Correct Background
	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
	// m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 19);

	// Custom Title
	hb::shared::text::draw_text(GameFont::Default, sX + 112, sY + 15, "Guild Interface", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

	short mx = static_cast<short>(hb::shared::input::get_mouse_x());
	short my = static_cast<short>(hb::shared::input::get_mouse_y());

	// Tabs
	hb::shared::render::Color tab_admin = (m_active_tab == 0) ? hb::shared::render::Color(255, 255, 255) : hb::shared::render::Color(100, 100, 100);
	hb::shared::render::Color tab_eco = (m_active_tab == 1) ? hb::shared::render::Color(255, 255, 255) : hb::shared::render::Color(100, 100, 100);
	hb::shared::render::Color tab_skills = (m_active_tab == 2) ? hb::shared::render::Color(255, 255, 255) : hb::shared::render::Color(100, 100, 100);

	if (mx >= sX + 27 && mx <= sX + 92 && my >= sY + 35 && my <= sY + 50 && m_active_tab != 0) tab_admin = hb::shared::render::Color(200, 200, 200);
	if (mx >= sX + 102 && mx <= sX + 172 && my >= sY + 35 && my <= sY + 50 && m_active_tab != 1) tab_eco = hb::shared::render::Color(200, 200, 200);
	if (mx >= sX + 182 && mx <= sX + 252 && my >= sY + 35 && my <= sY + 50 && m_active_tab != 2) tab_skills = hb::shared::render::Color(200, 200, 200);

	hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 35, "[Members]", hb::shared::text::TextStyle::with_shadow(tab_admin));
	hb::shared::text::draw_text(GameFont::Default, sX + 102, sY + 35, "[Economy]", hb::shared::text::TextStyle::with_shadow(tab_eco));
	hb::shared::text::draw_text(GameFont::Default, sX + 182, sY + 35, "[Passives]", hb::shared::text::TextStyle::with_shadow(tab_skills));

	if (m_active_tab == 0)
	{
		// Draw member list
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 65, "Guild Members:", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(200, 200, 200)));
		
		int max_pages = std::max(1, (m_guild_data.member_count + 11) / 12);
		if (m_current_page >= max_pages) m_current_page = max_pages - 1;
		if (m_current_page < 0) m_current_page = 0;

		std::string page_str = std::format("< {}/{} >", m_current_page + 1, max_pages);
		hb::shared::text::draw_text(GameFont::Default, sX + 192, sY + 65, page_str.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

		std::string sort_strs[] = {"Name", "Rank", "Map", "State"};
		std::string sort_str = std::format("Sort: {}", sort_strs[m_sort_method]);
		hb::shared::text::draw_text(GameFont::Default, sX + 122, sY + 65, sort_str.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(200, 200, 255)));

		int start_idx = m_current_page * 12;
		for (int i = 0; i < 12; ++i) {
			int member_idx = start_idx + i;
			if (member_idx >= m_guild_data.member_count) break;

			int listY = sY + 85 + i * 15;
			
			hb::shared::render::Color base_c = (m_selected_member == member_idx) ? hb::shared::render::Color(255, 255, 0) : hb::shared::render::Color(200, 200, 200);
			if (m_selected_member != member_idx && mx >= sX + 27 && mx <= sX + 252 && my >= listY && my <= listY + 15) base_c = hb::shared::render::Color(255, 255, 255);

			std::string rank_str = (m_guild_data.members[member_idx].rank == 1) ? "Master" : (m_guild_data.members[member_idx].rank == 2 ? "Officer" : "Member");

			int cur_x = sX + 27;
			hb::shared::text::draw_text(GameFont::Default, cur_x, listY, m_guild_data.members[member_idx].name, hb::shared::text::TextStyle::with_shadow(base_c));
			cur_x += hb::shared::text::measure_text(GameFont::Default, m_guild_data.members[member_idx].name).width + 5;

			hb::shared::text::draw_text(GameFont::Default, cur_x, listY, m_guild_data.members[member_idx].extra_info, hb::shared::text::TextStyle::with_shadow(base_c));
			cur_x += hb::shared::text::measure_text(GameFont::Default, m_guild_data.members[member_idx].extra_info).width + 5;

			hb::shared::text::draw_text(GameFont::Default, cur_x, listY, rank_str.c_str(), hb::shared::text::TextStyle::with_shadow(base_c));
			cur_x += hb::shared::text::measure_text(GameFont::Default, rank_str.c_str()).width + 5;

			hb::shared::render::Color status_c = hb::shared::render::Color(0, 255, 0); // Requested to be green for both online and offline
			const char* status_str = m_guild_data.members[member_idx].is_online ? "(Online)" : "(Offline)";
			hb::shared::text::draw_text(GameFont::Default, cur_x, listY, status_str, hb::shared::text::TextStyle::with_shadow(status_c));
		}

		// Action Buttons
		if (m_selected_member >= 0 && m_selected_member < m_guild_data.member_count) {
			if (get_my_guild_rank() <= 2) {
				hb::shared::render::Color btn = hb::shared::render::Color(200, 200, 200);
				hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 275, "[Promote]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 27 && mx <= sX + 92 && my >= sY + 275 && my <= sY + 290) ? hb::shared::render::Color(255,255,255) : btn));
				hb::shared::text::draw_text(GameFont::Default, sX + 102, sY + 275, "[Demote]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 102 && mx <= sX + 172 && my >= sY + 275 && my <= sY + 290) ? hb::shared::render::Color(255,255,255) : btn));
				hb::shared::text::draw_text(GameFont::Default, sX + 182, sY + 275, "[Kick]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 182 && mx <= sX + 242 && my >= sY + 275 && my <= sY + 290) ? hb::shared::render::Color(255,100,100) : btn));
			}
		}

		if (get_my_guild_rank() == 1) {
			hb::shared::text::draw_text(GameFont::Default, sX + 92, sY + 300, "[Disband Guild]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 92 && mx <= sX + 192 && my >= sY + 300 && my <= sY + 315) ? hb::shared::render::Color(255,100,100) : hb::shared::render::Color(200,50,50)));
		}
	}
	else if (m_active_tab == 1)
	{
		std::string tokens = std::format("Guild Tokens: {}", m_guild_data.tokens);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 65, tokens.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 0)));

		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 95, "[Donate 1,000 Gold]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 27 && mx <= sX + 162 && my >= sY + 95 && my <= sY + 110) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 115, "[Donate 10,000 Gold]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 27 && mx <= sX + 162 && my >= sY + 115 && my <= sY + 130) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));

		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 150, "Guild Shop:", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(200, 200, 200)));
		
		// Hardcoded items for UI
		int y_offset = 175;
		auto draw_item = [&](const char* name, int cost, int y) {
			std::string display = std::format("{} ({} T)", name, cost);
			hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + y, display.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
			hb::shared::text::draw_text(GameFont::Default, sX + 182, sY + y, "[Buy]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 182 && mx <= sX + 252 && my >= sY + y && my <= sY + y + 15) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));
		};

		draw_item("Exp Potion", 10, y_offset); y_offset += 20;
		draw_item("Super Exp", 25, y_offset); y_offset += 20;
		draw_item("Xelima", 50, y_offset); y_offset += 20;
		draw_item("Merien", 30, y_offset); y_offset += 20;
		draw_item("Zemstone", 100, y_offset);
	}
	else if (m_active_tab == 2)
	{
		std::string lvl_info = std::format("Guild Level: {} (GXP: {})", m_guild_data.guild_level, m_guild_data.guild_gxp);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 65, lvl_info.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 215, 0)));

		int total_spent = 0;
		for (int i = 0; i < 4; ++i) total_spent += m_guild_data.skills[i];
		int available_points = (m_guild_data.guild_level - 1) - total_spent;

		std::string pts_info = std::format("Available Points: {}", available_points);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 85, pts_info.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(200, 200, 255)));

		// Skill 1: Guerrero (HP, DMG)
		std::string s1 = std::format("1. Warrior (Level {})", m_guild_data.skills[0]);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 115, s1.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 150, 150)));
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 130, "   +50 Max HP per level.", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
		if (available_points > 0 && m_guild_data.skills[0] < 5)
			hb::shared::text::draw_text(GameFont::Default, sX + 222, sY + 115, "[+]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 222 && mx <= sX + 242 && my >= sY + 115 && my <= sY + 130) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));

		// Skill 2: Mago (MP, MagicRes)
		std::string s2 = std::format("2. Mage (Level {})", m_guild_data.skills[1]);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 155, s2.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 255)));
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 170, "   +50 Max MP per level.", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
		if (available_points > 0 && m_guild_data.skills[1] < 5)
			hb::shared::text::draw_text(GameFont::Default, sX + 222, sY + 155, "[+]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 222 && mx <= sX + 242 && my >= sY + 155 && my <= sY + 170) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));

		// Skill 3: Recolector
		std::string s3 = std::format("3. Gatherer (Level {})", m_guild_data.skills[2]);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 195, s3.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 255, 150)));
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 210, "   +10% Gathering Success.", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
		if (available_points > 0 && m_guild_data.skills[2] < 5)
			hb::shared::text::draw_text(GameFont::Default, sX + 222, sY + 195, "[+]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 222 && mx <= sX + 242 && my >= sY + 195 && my <= sY + 210) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));

		// Skill 4: Cazador
		std::string s4 = std::format("4. Hunter (Level {})", m_guild_data.skills[3]);
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 235, s4.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 150)));
		hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 250, "   +5% Global Drop Rate.", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(150, 150, 150)));
		if (available_points > 0 && m_guild_data.skills[3] < 5)
			hb::shared::text::draw_text(GameFont::Default, sX + 222, sY + 235, "[+]", hb::shared::text::TextStyle::with_shadow((mx >= sX + 222 && mx <= sX + 242 && my >= sY + 235 && my <= sY + 250) ? hb::shared::render::Color(255,255,255) : hb::shared::render::Color(200,200,200)));
		
		// Tooltips for passive skills
		item_tooltip tt;
		if (mx >= sX + 27 && mx <= sX + 212) {
			if (my >= sY + 115 && my <= sY + 145) {
				tt.add_line("Warrior (Passive)", hb::shared::render::Color(255, 150, 150));
				tt.add_line("Grants +50 Max HP and +2% Physical", hb::shared::render::Color(200, 200, 200));
				tt.add_line("Damage permanently to all members", hb::shared::render::Color(200, 200, 200));
				tt.add_line("for each passive level.", hb::shared::render::Color(200, 200, 200));
				tt.add_line(std::format("Current Bonus: +{} Max HP, +{}% Phys Dmg", m_guild_data.skills[0] * 50, m_guild_data.skills[0] * 2), hb::shared::render::Color(0, 255, 0));
			}
			else if (my >= sY + 155 && my <= sY + 185) {
				tt.add_line("Mage (Passive)", hb::shared::render::Color(150, 150, 255));
				tt.add_line("Grants +50 Max MP and +2% Magic", hb::shared::render::Color(200, 200, 200));
				tt.add_line("Damage permanently to all members", hb::shared::render::Color(200, 200, 200));
				tt.add_line("for each passive level.", hb::shared::render::Color(200, 200, 200));
				tt.add_line(std::format("Current Bonus: +{} Max MP, +{}% Magic Dmg", m_guild_data.skills[1] * 50, m_guild_data.skills[1] * 2), hb::shared::render::Color(0, 255, 0));
			}
			else if (my >= sY + 195 && my <= sY + 225) {
				tt.add_line("Gatherer (Passive)", hb::shared::render::Color(150, 255, 150));
				tt.add_line("Grants +10% Gathering success", hb::shared::render::Color(200, 200, 200));
				tt.add_line("(Mining/Fishing) permanently to all", hb::shared::render::Color(200, 200, 200));
				tt.add_line("members for each passive level.", hb::shared::render::Color(200, 200, 200));
				tt.add_line(std::format("Current Bonus: +{}% Gathering", m_guild_data.skills[2] * 10), hb::shared::render::Color(0, 255, 0));
			}
			else if (my >= sY + 235 && my <= sY + 265) {
				tt.add_line("Hunter (Passive)", hb::shared::render::Color(255, 255, 150));
				tt.add_line("Grants +5% Drop Rate and Gold Rate", hb::shared::render::Color(200, 200, 200));
				tt.add_line("permanently to all members", hb::shared::render::Color(200, 200, 200));
				tt.add_line("for each passive level.", hb::shared::render::Color(200, 200, 200));
				tt.add_line(std::format("Current Bonus: +{}% Drop Rate", m_guild_data.skills[3] * 5), hb::shared::render::Color(0, 255, 0));
			}
		}

		if (!tt.empty() && m_game->m_Renderer) {
			tt.draw(mx + 15, my + 15, m_game->m_Renderer);
		}
	}
}

bool DialogBox_Guild::on_click()
{
	short sX = m_x;
	short sY = m_y;
    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

	// Tabs
	if (mx >= sX + 27 && mx <= sX + 92 && my >= sY + 35 && my <= sY + 50) {
		m_active_tab = 0;
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}
	if (mx >= sX + 102 && mx <= sX + 172 && my >= sY + 35 && my <= sY + 50) {
		m_active_tab = 1;
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}
	if (mx >= sX + 182 && mx <= sX + 252 && my >= sY + 35 && my <= sY + 50) {
		m_active_tab = 2;
		audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		return true;
	}

	if (m_active_tab == 0)
	{
		int max_pages = std::max(1, (m_guild_data.member_count + 11) / 12);
		
		// Pagination
		if (my >= sY + 65 && my <= sY + 80) {
			if (mx >= sX + 192 && mx <= sX + 207) { // [<]
				if (m_current_page > 0) m_current_page--;
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
				return true;
			}
			if (mx >= sX + 227 && mx <= sX + 247) { // [>]
				if (m_current_page < max_pages - 1) m_current_page++;
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
				return true;
			}
			if (mx >= sX + 122 && mx <= sX + 192) { // Sort toggle
				m_sort_method = (m_sort_method + 1) % 4;
				sort_members();
				m_selected_member = -1;
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
				return true;
			}
		}

		// List clicks
		int start_idx = m_current_page * 12;
		for (int i = 0; i < 12; ++i) {
			int listY = sY + 85 + i * 15;
			if (mx >= sX + 27 && mx <= sX + 252 && my >= listY && my <= listY + 15) {
				int target_idx = start_idx + i;
				if (target_idx < m_guild_data.member_count) {
					m_selected_member = target_idx;
					audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
					return true;
				}
			}
		}

		// Action Buttons
		if (m_selected_member >= 0 && m_selected_member < m_guild_data.member_count) {
			if (get_my_guild_rank() <= 2) {
				// Promote
				if (mx >= sX + 27 && mx <= sX + 92 && my >= sY + 275 && my <= sY + 290) {
					PacketGuildAction action;
					std::memset(&action, 0, sizeof(action));
					action.msg_size = sizeof(action);
					action.header.msg_id = MsgId::GuildSystem;
					action.header.msg_type = GuildSystemType::ActionPromote;
					std::memcpy(action.target_name, m_guild_data.members[m_selected_member].name, 11);
					send_game_packet(action);
					audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
					return true;
				}
				
				// Demote
				if (mx >= sX + 102 && mx <= sX + 172 && my >= sY + 275 && my <= sY + 290) {
					PacketGuildAction action;
					std::memset(&action, 0, sizeof(action));
					action.msg_size = sizeof(action);
					action.header.msg_id = MsgId::GuildSystem;
					action.header.msg_type = GuildSystemType::ActionDemote;
					std::memcpy(action.target_name, m_guild_data.members[m_selected_member].name, 11);
					send_game_packet(action);
					audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
					return true;
				}

				// Kick
				if (mx >= sX + 182 && mx <= sX + 242 && my >= sY + 275 && my <= sY + 290) {
					PacketGuildAction action;
					std::memset(&action, 0, sizeof(action));
					action.msg_size = sizeof(action);
					action.header.msg_id = MsgId::GuildSystem;
					action.header.msg_type = GuildSystemType::ActionKick;
					std::memcpy(action.target_name, m_guild_data.members[m_selected_member].name, 11);
					send_game_packet(action);
					audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
					return true;
				}
			}
		}

		if (get_my_guild_rank() == 1) {
			if (mx >= sX + 92 && mx <= sX + 192 && my >= sY + 300 && my <= sY + 315) {
				m_game->get_dialog_box_manager().enable_dialog_box(DialogBoxId::GuildDisbandConfirm, 0, 0, 0);
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
				return true;
			}
		}
	}
	else if (m_active_tab == 1)
	{
		// Donate
		if (mx >= sX + 27 && mx <= sX + 162 && my >= sY + 95 && my <= sY + 110) {
			PacketGuildAction action;
			std::memset(&action, 0, sizeof(action));
			action.msg_size = sizeof(action);
			action.header.msg_id = MsgId::GuildSystem;
			action.header.msg_type = GuildSystemType::ActionDonate;

			action.amount = 1000;
			send_game_packet(action);
			audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			return true;
		}
		if (mx >= sX + 27 && mx <= sX + 162 && my >= sY + 115 && my <= sY + 130) {
			PacketGuildAction action;
			std::memset(&action, 0, sizeof(action));
			action.msg_size = sizeof(action);
			action.header.msg_id = MsgId::GuildSystem;
			action.header.msg_type = GuildSystemType::ActionDonate;
			action.amount = 10000;
			send_game_packet(action);
			audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			return true;
		}

		// Buy items
		auto send_buy = [&](const char* item_name) {
			PacketGuildAction action;
			std::memset(&action, 0, sizeof(action));
			action.msg_size = sizeof(action);
			action.header.msg_id = MsgId::GuildSystem;
			action.header.msg_type = GuildSystemType::ActionBuyItem;
			std::memcpy(action.item_name, item_name, std::min<size_t>(19, std::strlen(item_name)));
			send_game_packet(action);
			audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
		};

		if (mx >= sX + 182 && mx <= sX + 252) {
			if (my >= sY + 175 && my <= sY + 190) { send_buy("xp"); return true; }
			if (my >= sY + 195 && my <= sY + 210) { send_buy("superxp"); return true; }
			if (my >= sY + 215 && my <= sY + 230) { send_buy("xelima"); return true; }
			if (my >= sY + 235 && my <= sY + 250) { send_buy("merien"); return true; }
			if (my >= sY + 255 && my <= sY + 270) { send_buy("zemstone"); return true; }
		}
	}
	else if (m_active_tab == 2)
	{
		int total_spent = 0;
		for (int i = 0; i < 4; ++i) total_spent += m_guild_data.skills[i];
		int available_points = (m_guild_data.guild_level - 1) - total_spent;

		if (available_points > 0) {
			auto upgrade_skill = [&](int skill_id) {
				PacketGuildAction action;
				std::memset(&action, 0, sizeof(action));
				action.msg_size = sizeof(action);
				action.header.msg_id = MsgId::GuildSystem;
				action.header.msg_type = GuildSystemType::ActionUpgradeSkill;
				action.amount = skill_id; // Pass skill ID via amount
				send_game_packet(action);
				audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
			};

			if (m_guild_data.skills[0] < 5 && mx >= sX + 222 && mx <= sX + 242 && my >= sY + 115 && my <= sY + 130) { upgrade_skill(1); return true; }
			if (m_guild_data.skills[1] < 5 && mx >= sX + 222 && mx <= sX + 242 && my >= sY + 155 && my <= sY + 170) { upgrade_skill(2); return true; }
			if (m_guild_data.skills[2] < 5 && mx >= sX + 222 && mx <= sX + 242 && my >= sY + 195 && my <= sY + 210) { upgrade_skill(3); return true; }
			if (m_guild_data.skills[3] < 5 && mx >= sX + 222 && mx <= sX + 242 && my >= sY + 235 && my <= sY + 250) { upgrade_skill(4); return true; }
		}
	}

	return false;
}

bool DialogBox_Guild::on_enable(int type, int64_t v1, int v2, const char* string)
{
	if (!m_has_been_opened) {
		auto* char_dlg = m_game->get_dialog_box_manager().get_dialog_box(DialogBoxId::CharacterInfo);
		if (char_dlg) {
			m_x = char_dlg->m_x + char_dlg->m_size_x + 5;
			m_y = char_dlg->m_y;
		}
		m_has_been_opened = true;
	}

	m_active_tab = 0;
	m_selected_member = -1;

	// Request guild info from server
	hb::net::packet_base pkt;
	std::memset(&pkt, 0, sizeof(pkt));
	auto* header = reinterpret_cast<hb::net::PacketHeader*>(&pkt);
	header->msg_id = MsgId::GuildSystem;
	header->msg_type = GuildSystemType::RequestMembers;
	send_game_packet_impl(pkt, sizeof(hb::net::PacketHeader), true);

	return true;
}

bool DialogBox_Guild::on_disable()
{
	return true;
}
