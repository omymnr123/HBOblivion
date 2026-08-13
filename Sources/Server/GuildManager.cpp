#include "GuildManager.h"
#include "GuildSqliteStore.h"
#include "Game.h"
#include "Client.h"
#include "ItemManager.h"
#include "Item.h"
#include "Log.h"
#include "sqlite3.h"
#include "OwnerClass.h"
#include "ActionID.h"
#include <iostream>
#include <format>
#include <vector>
#include <algorithm>
#include "../Dependencies/Shared/Packet/PacketGuildSystem.h"

using namespace hb::shared::net;

bool GuildManager::initialize()
{
	std::string path;
	if (!EnsureGuildDatabase(&m_db, path, m_game)) {
		hb::logger::log("Failed to initialize Guild Database.");
		return false;
	}
	load_guild_skills_cache();
	hb::logger::log("Guild Database initialized at {}", path);
	return true;
}

void GuildManager::cleanup()
{
	CloseGuildDatabase(m_db);
	m_db = nullptr;
}

void GuildManager::get_player_guild_info(const std::string& char_name, uint32_t& out_guid, int& out_rank)
{
	if (!m_db) return;
	GetMemberInfo(m_db, char_name, out_guid, out_rank);
}

void GuildManager::update_member_login(const char* char_name, uint32_t timestamp)
{
	if (!m_db) return;
	const char* sql = "UPDATE guild_members SET last_login = ? WHERE character_name = ?;";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, timestamp);
		sqlite3_bind_text(stmt, 2, char_name, -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void GuildManager::create_guild(int client_h, const std::string& guild_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;

	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid != 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are already in a guild.");
		return;
	}

	if (client->m_level < 10) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You must be level 10 to create a guild.");
		return;
	}

	// Check gold via ItemManager
	uint64_t gold_count = m_game->m_item_manager->get_item_count_by_id(client_h, hb::shared::item::ItemId::Gold);
	if (gold_count < 30000) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You need 30,000 gold to create a guild.");
		return;
	}

	uint32_t new_guid = 0;
	if (!CreateGuild(m_db, guild_name, client->m_char_name, new_guid)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Failed to create guild. Name might be taken.");
		return;
	}

	// Add master
	if (!AddGuildMember(m_db, client->m_char_name, new_guid, static_cast<int>(GuildRank::Master))) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Database error adding guild master.");
		return;
	}

	// Deduct gold
	m_game->m_item_manager->set_item_count_by_id(client_h, hb::shared::item::ItemId::Gold, gold_count - 30000);
	m_game->send_notify_msg(0, client_h, Notify::RewardGold, 0, 0, 0, 0);

	client->m_guild_guid = new_guid;
	client->m_guild_rank = static_cast<int>(GuildRank::Master);

	client->m_status.guild_rank = static_cast<int8_t>(client->m_guild_rank);
	strncpy(client->m_status.guild_name, guild_name.c_str(), sizeof(client->m_status.guild_name) - 1);
	client->m_status.guild_name[sizeof(client->m_status.guild_name) - 1] = '\0';

	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Guild created successfully!");
	broadcast_guild_visual_update(client_h);
}

void GuildManager::invite_member(int client_h, const std::string& target_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank > static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Master and Officers can invite.");
		return;
	}

	int target_h = m_game->find_client_by_name(target_name.c_str());
	if (target_h == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player not found or offline.");
		return;
	}

	CClient* target = m_game->m_client_list[target_h];
	if (target->m_guild_guid != 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player is already in a guild.");
		return;
	}

	m_pending_invites[target_h] = client_h;

	hb::shared::net::PacketGuildNotifyInvite invite_pkt{};
	invite_pkt.header.msg_id = MsgId::GuildSystem;
	invite_pkt.header.msg_type = hb::shared::net::GuildSystemType::NotifyInvite;
	invite_pkt.msg_size = sizeof(invite_pkt);
	
	std::snprintf(invite_pkt.inviter_name, sizeof(invite_pkt.inviter_name), "%s", client->m_char_name);
	std::snprintf(invite_pkt.guild_name, sizeof(invite_pkt.guild_name), "%s", client->m_status.guild_name);
	
	CClient* target_client = m_game->m_client_list[target_h];
	if (target_client && target_client->m_socket) {
		target_client->m_socket->send_msg(reinterpret_cast<char*>(&invite_pkt), invite_pkt.msg_size);
	}

	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Invitation sent.");
}

void GuildManager::accept_invite(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	auto it = m_pending_invites.find(client_h);
	if (it == m_pending_invites.end()) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You have no pending guild invitations.");
		return;
	}

	int inviter_h = it->second;
	m_pending_invites.erase(it);

	if (m_game->m_client_list[inviter_h] == nullptr) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "The inviter is no longer online.");
		return;
	}

	CClient* inviter = m_game->m_client_list[inviter_h];
	if (inviter->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "The inviter is no longer in a guild.");
		return;
	}

	if (!AddGuildMember(m_db, client->m_char_name, inviter->m_guild_guid, static_cast<int>(GuildRank::Member))) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Database error joining guild.");
		return;
	}

	client->m_guild_guid = inviter->m_guild_guid;
	client->m_guild_rank = static_cast<int>(GuildRank::Member);
	client->m_status.guild_rank = static_cast<int8_t>(client->m_guild_rank);
	strncpy(client->m_status.guild_name, inviter->m_status.guild_name, sizeof(client->m_status.guild_name) - 1);
	client->m_status.guild_name[sizeof(client->m_status.guild_name) - 1] = '\0';

	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You have joined the guild.");
	m_game->send_notify_msg(0, inviter_h, Notify::NoticeMsg, 0, 0, 0, (std::string(client->m_char_name) + " has joined the guild.").c_str());
	broadcast_guild_visual_update(client_h);
}

void GuildManager::kick_member(int client_h, const std::string& target_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank > static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Master and Officers can kick members.");
		return;
	}

	uint32_t target_guid = 0;
	int target_rank = 0;
	if (!GetMemberInfo(m_db, target_name, target_guid, target_rank)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Member not found in database.");
		return;
	}

	if (target_guid != client->m_guild_guid) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player is not in your guild.");
		return;
	}

	if (target_rank <= client->m_guild_rank) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You cannot kick someone of equal or higher rank.");
		return;
	}

	if (!RemoveGuildMember(m_db, target_name)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Database error removing member.");
		return;
	}

	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Member kicked successfully.");

	// Update online player if connected
	int target_h = m_game->find_client_by_name(target_name.c_str());
	if (target_h != 0 && m_game->m_client_list[target_h] != nullptr) {
		m_game->m_client_list[target_h]->m_guild_guid = 0;
		m_game->m_client_list[target_h]->m_guild_rank = 0;
		m_game->m_client_list[target_h]->m_status.guild_rank = 0;
		m_game->m_client_list[target_h]->m_status.guild_name[0] = '\0';
		m_game->send_notify_msg(0, target_h, Notify::NoticeMsg, 0, 0, 0, "You have been kicked from the guild.");
		broadcast_guild_visual_update(target_h);
	}
}

void GuildManager::leave_guild(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank == static_cast<int>(GuildRank::Master)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "The Master cannot leave. Disband the guild or pass leadership first.");
		return;
	}

	if (!RemoveGuildMember(m_db, client->m_char_name)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Database error leaving guild.");
		return;
	}

	client->m_guild_guid = 0;
	client->m_guild_rank = 0;
	client->m_status.guild_rank = 0;
	client->m_status.guild_name[0] = '\0';
	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You have left the guild.");
	broadcast_guild_visual_update(client_h);
}

void GuildManager::broadcast_guild_chat(int client_h, const char* message)
{
	if (!m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) return;

	// This is just a backup mechanism; normally it is handled directly in Game.cpp's MsgId::CommandChatMsg
}

void GuildManager::add_guild_gxp(uint32_t guild_guid, int amount)
{
	if (!m_db || guild_guid == 0 || amount <= 0) return;

	int level = 1, gxp = 0;
	if (GetGuildProgression(m_db, guild_guid, level, gxp)) {
		gxp += amount;
		bool level_up = false;
		
		while (level < GuildConfig::MAX_GUILD_LEVEL) {
			int req = GuildConfig::get_gxp_requirement(level + 1);
			if (gxp >= req) {
				level++;
				level_up = true;
			} else {
				break;
			}
		}

		UpdateGuildProgression(m_db, guild_guid, level, gxp);
		
		if (level_up) {
			// Notify all members of level up
			for (int i = 0; i < hb::server::config::MaxClients; ++i) {
				if (m_game->m_client_list[i] && m_game->m_client_list[i]->m_guild_guid == guild_guid) {
					std::string msg = std::format("Your Guild has reached Level {}!", level);
					m_game->send_notify_msg(0, i, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
					m_game->send_notify_msg(0, i, hb::shared::net::Notify::GuildLevelUp, 0, 0, 0, 0);
				}
			}
			// --- NUEVO: Actualizar interfaz al instante para ver los puntos de habilidad ---
			broadcast_guild_info_update(guild_guid);
		}
	}
}

void GuildManager::add_member_tokens(int client_h, int amount)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr || amount <= 0) return;
	CClient* client = m_game->m_client_list[client_h];
	
	const char* sql = "UPDATE guild_members SET contribution = contribution + ? WHERE character_name = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, amount);
		sqlite3_bind_text(stmt, 2, client->m_char_name, -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void GuildManager::process_donate_command(int client_h, int gold_amount)
{
	if (!m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (gold_amount <= 0) {
		m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Invalid donation amount.");
		return;
	}

	uint64_t gold_count = m_game->m_item_manager->get_item_count_by_id(client_h, hb::shared::item::ItemId::Gold);
	if (gold_count < gold_amount) {
		m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You do not have enough gold.");
		return;
	}

	// Deduct gold
	m_game->m_item_manager->set_item_count_by_id(client_h, hb::shared::item::ItemId::Gold, gold_count - gold_amount);
	
	int gxp_gained = gold_amount / GuildConfig::GOLD_PER_GXP;
	int tokens_gained = gold_amount / 1000;

	if (gxp_gained > 0) add_guild_gxp(client->m_guild_guid, gxp_gained);
	if (tokens_gained > 0) add_member_tokens(client_h, tokens_gained);

	std::string msg = std::format("You donated {} gold. GXP: +{}, Tokens: +{}", gold_amount, gxp_gained, tokens_gained);
	m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
	send_guild_info_to_client(client_h);
}

void GuildManager::show_guild_info(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	// Fetch guild info
	int level = 1;
	int gxp = 0;
	int tokens = 0;
	int contribution = 0;

	const char* sql_guild = "SELECT guild_level, guild_gxp FROM guilds WHERE guid = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql_guild, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, client->m_guild_guid);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			level = sqlite3_column_int(stmt, 0);
			gxp = sqlite3_column_int(stmt, 1);
		}
		sqlite3_finalize(stmt);
	}

	const char* sql_member = "SELECT contribution FROM guild_members WHERE character_name = ?;";
	if (sqlite3_prepare_v2(m_db, sql_member, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, client->m_char_name, -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			contribution = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	std::string msg1 = std::format("Guild Level: {}  GXP: {} / {}", level, gxp, level * 1000);
	std::string msg2 = std::format("Your Guild Tokens: {}", contribution);

	m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg1.c_str());
	m_game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg2.c_str());
}

void GuildManager::broadcast_guild_visual_update(int client_h)
{
	if (!m_game || m_game->m_client_list[client_h] == nullptr) return;
	m_game->send_event_to_near_client_type_a(static_cast<short>(client_h), hb::shared::owner_class::Player, hb::shared::net::MsgId::EventMotion, hb::shared::action::Type::NullAction, 0, 0, 0);
}

void GuildManager::promote_member(int client_h, const std::string& target_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank > static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Master and Officers can promote members.");
		return;
	}

	uint32_t target_guid = 0;
	int target_rank = 0;
	if (!GetMemberInfo(m_db, target_name, target_guid, target_rank)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Member not found.");
		return;
	}

	if (target_guid != client->m_guild_guid) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player is not in your guild.");
		return;
	}

	if (target_rank <= static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player is already Officer or higher.");
		return;
	}

	// Promote: Member(3) -> Officer(2)
	const char* sql = "UPDATE guild_members SET guild_rank = ? WHERE character_name = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, static_cast<int>(GuildRank::Officer));
		sqlite3_bind_text(stmt, 2, target_name.c_str(), -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	// Update online player if connected
	int target_h = m_game->find_client_by_name(target_name.c_str());
	if (target_h != 0 && m_game->m_client_list[target_h] != nullptr) {
		m_game->m_client_list[target_h]->m_guild_rank = static_cast<int>(GuildRank::Officer);
		m_game->m_client_list[target_h]->m_status.guild_rank = static_cast<int8_t>(GuildRank::Officer);
		m_game->send_notify_msg(0, target_h, Notify::NoticeMsg, 0, 0, 0, "You have been promoted to Officer!");
		broadcast_guild_visual_update(target_h);
	}

	std::string msg = target_name + " has been promoted to Officer.";
	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg.c_str());
	send_guild_info_to_client(client_h);
}

void GuildManager::demote_member(int client_h, const std::string& target_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank > static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Master and Officers can demote members.");
		return;
	}

	uint32_t target_guid = 0;
	int target_rank = 0;
	if (!GetMemberInfo(m_db, target_name, target_guid, target_rank)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Member not found.");
		return;
	}

	if (target_guid != client->m_guild_guid) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Player is not in your guild.");
		return;
	}

	if (target_rank != static_cast<int>(GuildRank::Officer)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only Officers can be demoted.");
		return;
	}

	// Demote: Officer(2) -> Member(3)
	const char* sql = "UPDATE guild_members SET guild_rank = ? WHERE character_name = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, static_cast<int>(GuildRank::Member));
		sqlite3_bind_text(stmt, 2, target_name.c_str(), -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	// Update online player if connected
	int target_h = m_game->find_client_by_name(target_name.c_str());
	if (target_h != 0 && m_game->m_client_list[target_h] != nullptr) {
		m_game->m_client_list[target_h]->m_guild_rank = static_cast<int>(GuildRank::Member);
		m_game->m_client_list[target_h]->m_status.guild_rank = static_cast<int8_t>(GuildRank::Member);
		m_game->send_notify_msg(0, target_h, Notify::NoticeMsg, 0, 0, 0, "You have been demoted to Member.");
		broadcast_guild_visual_update(target_h);
	}

	std::string msg = target_name + " has been demoted to Member.";
	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg.c_str());
	send_guild_info_to_client(client_h);
}

void GuildManager::disband_guild(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	if (client->m_guild_rank != static_cast<int>(GuildRank::Master)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Guild Master can disband the guild.");
		return;
	}

	uint32_t guild_guid = client->m_guild_guid;

	// Clean up all online members
	for (int i = 1; i < hb::server::config::MaxClients; i++) {
		if (m_game->m_client_list[i] != nullptr && m_game->m_client_list[i]->m_guild_guid == guild_guid) {
			m_game->m_client_list[i]->m_guild_guid = 0;
			m_game->m_client_list[i]->m_guild_rank = 0;
			m_game->m_client_list[i]->m_status.guild_rank = 0;
			m_game->m_client_list[i]->m_status.guild_name[0] = '\0';
			if (i != client_h) {
				m_game->send_notify_msg(0, i, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Your guild has been disbanded.");
			}
			broadcast_guild_visual_update(i);

			// Clamp HP and MP to the new maxes now that guild passives are gone
			m_game->m_client_list[i]->m_hp = std::min(m_game->m_client_list[i]->m_hp, m_game->get_max_hp(i));
			m_game->m_client_list[i]->m_mp = std::min(m_game->m_client_list[i]->m_mp, m_game->get_max_mp(i));
			m_game->send_event_to_near_client_type_a(i, hb::shared::owner_class::Player, hb::shared::net::MsgId::EventMotion, hb::shared::action::Type::Damage, 0, 0, 0); // Trigger UI update for HP/MP if needed
		}
	}

	// Delete from database (cascades to members due to FK)
	DisbandGuild(m_db, guild_guid);

	// Also clean guild_members table explicitly in case FK cascade didn't work
	const char* sql = "DELETE FROM guild_members WHERE guild_guid = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, guild_guid);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Guild disbanded.");
	send_guild_info_to_client(client_h);
}

void GuildManager::list_members(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	auto members = GetGuildMembers(m_db, client->m_guild_guid);

	std::string guild_name;
	GetGuildName(m_db, client->m_guild_guid, guild_name);

	std::string header = std::format("=== {} Members ({}) ===", guild_name, members.size());
	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, header.c_str());

	for (const auto& m : members) {
		std::string rank_str;
		switch (m.guild_rank) {
		case 1: rank_str = "Master"; break;
		case 2: rank_str = "Officer"; break;
		default: rank_str = "Member"; break;
		}

		// Check if online
		int online_h = m_game->find_client_by_name(m.character_name.c_str());
		std::string status = (online_h != 0) ? "Online" : "Offline";

		std::string line = std::format("  {} [{}] - {}", m.character_name, rank_str, status);
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, line.c_str());
	}
}

void GuildManager::process_shop_command(int client_h, const std::string& item_name)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "You are not in a guild.");
		return;
	}

	// Shop catalog: keyword -> { item_id, count, token_cost, display_name }
	struct ShopEntry {
		int item_id;
		int count;
		int token_cost;
		int min_level; // Minimum guild level required
		const char* display_name;
	};

	static const std::map<std::string, ShopEntry> shop_catalog = {
		{"xp",      {1112,                                      1,  10,  1,  "Experience Potion"}},
		{"superxp", {1113,                                      1,  25,  5,  "Super Experience Potion"}},
		{"zemstone",{hb::shared::item::ItemId::ZemstoneofSacrifice,1, 100, 10, "Zemstone of Sacrifice"}},
		{"xelima",  {hb::shared::item::ItemId::StoneOfXelima,  1,  50,  15, "Stone of Xelima"}},
		{"merien",  {hb::shared::item::ItemId::StoneOfMerien,  1,  30,  15, "Stone of Merien"}},
	};

	// Fetch guild level
	int guild_level = 1;
	const char* sql_guild = "SELECT guild_level FROM guilds WHERE guid = ?;";
	sqlite3_stmt* stmt_g = nullptr;
	if (sqlite3_prepare_v2(m_db, sql_guild, -1, &stmt_g, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt_g, 1, client->m_guild_guid);
		if (sqlite3_step(stmt_g) == SQLITE_ROW) {
			guild_level = sqlite3_column_int(stmt_g, 0);
		}
		sqlite3_finalize(stmt_g);
	}

	// If no item specified, show the catalog
	if (item_name.empty()) {
		// Get player's current tokens
		int tokens = 0;
		const char* sql_tok = "SELECT contribution FROM guild_members WHERE character_name = ?";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(m_db, sql_tok, -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_text(stmt, 1, client->m_char_name, -1, SQLITE_STATIC);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				tokens = sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		}

		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "=== Guild Quartermaster ===");
		std::string tok_msg = std::format("Your Guild Tokens: {} | Guild Level: {}", tokens, guild_level);
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, tok_msg.c_str());

		for (const auto& [key, entry] : shop_catalog) {
			if (guild_level >= entry.min_level) {
				std::string line = std::format("  /guild shop {} - {} (x{}) = {} tokens", key, entry.display_name, entry.count, entry.token_cost);
				m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, line.c_str());
			} else {
				std::string line = std::format("  [Locked Lvl {}] - {}", entry.min_level, entry.display_name);
				m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, line.c_str());
			}
		}
		return;
	}

	// Find item in catalog
	auto it = shop_catalog.find(item_name);
	if (it == shop_catalog.end()) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Unknown item. Type /guild shop to see available items.");
		return;
	}

	const ShopEntry& entry = it->second;

	if (guild_level < entry.min_level) {
		std::string msg = std::format("Your guild must be level {} to buy this item.", entry.min_level);
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg.c_str());
		return;
	}

	// Check tokens
	int tokens = 0;
	const char* sql_tok = "SELECT contribution FROM guild_members WHERE character_name = ?;";
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(m_db, sql_tok, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, client->m_char_name, -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			tokens = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}

	if (tokens < entry.token_cost) {
		std::string msg = std::format("Not enough tokens. Need {} but you have {}.", entry.token_cost, tokens);
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg.c_str());
		return;
	}

	// Create item and give to player
	CItem* item = new CItem;
	if (!m_game->m_item_manager->init_item_attr(item, entry.item_id)) {
		delete item;
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Error creating item.");
		return;
	}
	item->m_instance.count = entry.count;

	if (!m_game->m_item_manager->check_item_receive_condition(client_h, item)) {
		delete item;
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Your inventory is full.");
		return;
	}

	int erase_req = 0;
	m_game->m_item_manager->add_client_item_list(client_h, item, &erase_req);
	m_game->m_item_manager->send_item_notify_msg(client_h, Notify::ItemObtained, item, 0);
	if (erase_req == 1) delete item;

	// Deduct tokens
	const char* sql_deduct = "UPDATE guild_members SET contribution = contribution - ? WHERE character_name = ?";
	if (sqlite3_prepare_v2(m_db, sql_deduct, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, entry.token_cost);
		sqlite3_bind_text(stmt, 2, client->m_char_name, -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	std::string msg = std::format("Purchased {} (x{}) for {} tokens.", entry.display_name, entry.count, entry.token_cost);
	m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg.c_str());
	send_guild_info_to_client(client_h);
}

void GuildManager::send_guild_info_to_client(int client_h)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;

	CClient* client = m_game->m_client_list[client_h];

	// Prepare packet
	PacketGuildMemberList pkt;
	std::memset(&pkt, 0, sizeof(pkt));
	pkt.header.msg_id = MsgId::GuildSystem;
	pkt.header.msg_type = GuildSystemType::ResponseMembers;
	pkt.tokens = 0;
	pkt.member_count = 0;

	if (client->m_guild_guid == 0) {
		pkt.msg_size = sizeof(PacketGuildMemberList);
		client->m_socket->send_msg(reinterpret_cast<char*>(&pkt), sizeof(PacketGuildMemberList));
		return;
	}

	// Get Guild tokens, level, gxp, and skills
	uint32_t tokens = 0;
	{
		std::string sql = std::format("SELECT contribution FROM guild_members WHERE character_name = '{}';", client->m_char_name);
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				tokens = sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		}
	}
	pkt.tokens = tokens;
	
	pkt.guild_level = get_guild_level(client->m_guild_guid);
	
	int gxp = 0, lvl = 0;
	GetGuildProgression(m_db, client->m_guild_guid, lvl, gxp);
	pkt.guild_gxp = gxp;

	for (int i = 0; i < 4; ++i) {
		pkt.skills[i] = get_guild_skill_level(client->m_guild_guid, i + 1); // SkillId is 1-based
	}

	// Query members
	std::string sql = std::format("SELECT character_name, guild_rank, last_login FROM guild_members WHERE guild_guid = {};", client->m_guild_guid);
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW && pkt.member_count < 100) {
			const char* char_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			int rank = sqlite3_column_int(stmt, 1);
			uint32_t last_login = sqlite3_column_int(stmt, 2);

			if (char_name) {
				std::strncpy(pkt.members[pkt.member_count].name, char_name, 11);
				pkt.members[pkt.member_count].name[11] = '\0';
				pkt.members[pkt.member_count].rank = static_cast<std::uint8_t>(rank);
				
				// Check if online
				pkt.members[pkt.member_count].is_online = false;
				for (int i = 0; i < hb::server::config::MaxClients; ++i) {
					if (m_game->m_client_list[i] && hb_stricmp(m_game->m_client_list[i]->m_char_name, char_name) == 0) {
						pkt.members[pkt.member_count].is_online = true;
						std::strncpy(pkt.members[pkt.member_count].extra_info, m_game->m_client_list[i]->m_map_name, 19);
						pkt.members[pkt.member_count].extra_info[19] = '\0';
						break;
					}
				}

				if (!pkt.members[pkt.member_count].is_online) {
					if (last_login == 0) {
						std::strncpy(pkt.members[pkt.member_count].extra_info, "Never", 19);
					} else {
						time_t t = static_cast<time_t>(last_login);
						struct tm* tm_info = localtime(&t);
						if (tm_info) {
							strftime(pkt.members[pkt.member_count].extra_info, 20, "%d-%m-%Y", tm_info);
						} else {
							std::strncpy(pkt.members[pkt.member_count].extra_info, "Unknown", 19);
						}
					}
				}

				pkt.member_count++;
			}
		}
		sqlite3_finalize(stmt);
	}

	pkt.msg_size = sizeof(PacketGuildMemberList);
	m_game->m_client_list[client_h]->m_socket->send_msg(reinterpret_cast<char*>(&pkt), sizeof(PacketGuildMemberList));
}

void GuildManager::load_guild_skills_cache()
{
	if (!m_db) return;
	m_guild_skills_cache.clear();
	const char* sql = "SELECT guid FROM guilds";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			uint32_t guid = sqlite3_column_int(stmt, 0);
			auto skills = GetGuildSkills(m_db, guid);
			for (const auto& skill : skills) {
				m_guild_skills_cache[guid][skill.first] = skill.second;
			}
		}
		sqlite3_finalize(stmt);
	}
}

int GuildManager::get_guild_level(uint32_t guild_guid)
{
	int level = 1, gxp = 0;
	if (GetGuildProgression(m_db, guild_guid, level, gxp)) {
		return level;
	}
	return 1;
}

int GuildManager::get_guild_skill_level(uint32_t guild_guid, int skill_id)
{
	if (m_guild_skills_cache.count(guild_guid)) {
		if (m_guild_skills_cache[guild_guid].count(skill_id)) {
			return m_guild_skills_cache[guild_guid][skill_id];
		}
	}
	return 0;
}

int GuildManager::get_player_guild_skill(int client_h, int skill_id)
{
	if (!m_game || m_game->m_client_list[client_h] == nullptr) return 0;
	uint32_t guid = m_game->m_client_list[client_h]->m_guild_guid;
	if (guid == 0) return 0;
	return get_guild_skill_level(guid, skill_id);
}

void GuildManager::process_upgrade_skill_command(int client_h, int skill_id)
{
	if (!m_db || !m_game || m_game->m_client_list[client_h] == nullptr) return;
	CClient* client = m_game->m_client_list[client_h];

	if (client->m_guild_guid == 0 || client->m_guild_rank != static_cast<int>(GuildRank::Master)) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Only the Guild Master can upgrade skills.");
		return;
	}

	int level = 1, gxp = 0;
	if (!GetGuildProgression(m_db, client->m_guild_guid, level, gxp)) return;

	int total_spent = 0;
	for (int i = 1; i <= 4; ++i) {
		total_spent += get_guild_skill_level(client->m_guild_guid, i);
	}

	int available_points = (level - 1) - total_spent;
	if (available_points <= 0) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "No skill points available.");
		return;
	}

	int current_skill_lvl = get_guild_skill_level(client->m_guild_guid, skill_id);
	if (current_skill_lvl >= GuildConfig::MAX_SKILL_LEVEL) {
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "This skill is already at max level.");
		return;
	}

	if (SetGuildSkill(m_db, client->m_guild_guid, skill_id, current_skill_lvl + 1)) {
		m_guild_skills_cache[client->m_guild_guid][skill_id] = current_skill_lvl + 1;
		m_game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Guild skill upgraded!");
		broadcast_guild_info_update(client->m_guild_guid);
		
		// Update everyone's HP/MP limits
		for (int i = 0; i < hb::server::config::MaxClients; ++i) {
			if (m_game->m_client_list[i] && m_game->m_client_list[i]->m_guild_guid == client->m_guild_guid) {
				m_game->m_client_list[i]->m_hp = std::min(m_game->m_client_list[i]->m_hp, m_game->get_max_hp(i));
				m_game->m_client_list[i]->m_mp = std::min(m_game->m_client_list[i]->m_mp, m_game->get_max_mp(i));
			}
		}
	}
}

void GuildManager::broadcast_guild_info_update(uint32_t guild_guid)
{
	for (int i = 0; i < hb::server::config::MaxClients; ++i) {
		if (m_game->m_client_list[i] && m_game->m_client_list[i]->m_guild_guid == guild_guid) {
			send_guild_info_to_client(i);
		}
	}
}

void GuildManager::update(uint32_t current_time)
{
	if (!m_game) return;

	// Se ejecuta cada 60.000 milisegundos (1 minuto)
	if (current_time - m_last_broadcast_time >= 60000) {
		m_last_broadcast_time = current_time;

		// Guardamos a que clanes ya les hemos enviado datos para no repetir
		std::vector<uint32_t> updated_guilds;

		for (int i = 1; i < hb::server::config::MaxClients; i++) {
			CClient* client = m_game->m_client_list[i];
			if (client && client->m_is_init_complete && client->m_guild_guid != 0) {
				// Si no hemos actualizado este clan en este barrido, lo actualizamos
				if (std::find(updated_guilds.begin(), updated_guilds.end(), client->m_guild_guid) == updated_guilds.end()) {
					updated_guilds.push_back(client->m_guild_guid);
					broadcast_guild_info_update(client->m_guild_guid);
				}
			}
		}
	}
}