#include "GameCmdGuild.h"
#include "Game.h"
#include "GuildManager.h"
#include "Log.h"
#include "StringCompat.h"
#include <cstring>
#include <sstream>

using namespace hb::shared::net;

bool GameCmdGuild::execute(CGame* game, int client_h, const char* args)
{
	if (!game || !game->m_guild_manager)
		return true;

	if (!game->m_client_list[client_h])
		return true;

	// No args: show help
	if (args == nullptr || args[0] == '\0')
	{
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Guild Commands:");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild create <Name> - Create a guild");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild invite <Player> - Invite a player");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild accept - Accept an invitation");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild kick <Player> - Kick a member");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild leave - Leave your guild");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild info - Show Guild and Member stats");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild donate <amount> - Donate gold for GXP/Tokens");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild shop [item] - Open Guild Shop (Tokens)");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild promote <Player> - Promote to Officer");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild demote <Player> - Demote to Member");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild members - List all guild members");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "/guild disband - Disband your guild");
		return true;
	}

	// Parse sub-command
	std::istringstream iss(args);
	std::string subcmd;
	iss >> subcmd;

	if (hb_strnicmp(subcmd.c_str(), "create", 6) == 0)
	{
		std::string guild_name;
		iss >> guild_name;
		if (guild_name.empty())
		{
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /guild create <Name>");
			return true;
		}
		game->m_guild_manager->create_guild(client_h, guild_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "invite", 6) == 0)
	{
		std::string target_name;
		iss >> target_name;
		if (target_name.empty())
		{
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /guild invite <PlayerName>");
			return true;
		}
		game->m_guild_manager->invite_member(client_h, target_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "accept", 6) == 0)
	{
		game->m_guild_manager->accept_invite(client_h);
	}
	else if (hb_strnicmp(subcmd.c_str(), "kick", 4) == 0)
	{
		std::string target_name;
		iss >> target_name;
		if (target_name.empty())
		{
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /guild kick <PlayerName>");
			return true;
		}
		game->m_guild_manager->kick_member(client_h, target_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "leave", 5) == 0)
	{
		game->m_guild_manager->leave_guild(client_h);
	}
	else if (hb_strnicmp(subcmd.c_str(), "info", 4) == 0)
	{
		game->m_guild_manager->show_guild_info(client_h);
	}
	else if (hb_strnicmp(subcmd.c_str(), "donate", 6) == 0)
	{
		int amount = 0;
		iss >> amount;
		game->m_guild_manager->process_donate_command(client_h, amount);
	}
	else if (hb_strnicmp(subcmd.c_str(), "shop", 4) == 0)
	{
		std::string item_name;
		iss >> item_name;
		game->m_guild_manager->process_shop_command(client_h, item_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "promote", 7) == 0)
	{
		std::string target_name;
		iss >> target_name;
		if (target_name.empty())
		{
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /guild promote <PlayerName>");
			return true;
		}
		game->m_guild_manager->promote_member(client_h, target_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "demote", 6) == 0)
	{
		std::string target_name;
		iss >> target_name;
		if (target_name.empty())
		{
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Usage: /guild demote <PlayerName>");
			return true;
		}
		game->m_guild_manager->demote_member(client_h, target_name);
	}
	else if (hb_strnicmp(subcmd.c_str(), "disband", 7) == 0)
	{
		game->m_guild_manager->disband_guild(client_h);
	}
	else if (hb_strnicmp(subcmd.c_str(), "members", 7) == 0)
	{
		game->m_guild_manager->list_members(client_h);
	}
	else
	{
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, "Unknown guild command. Type /guild for help.");
	}

	return true;
}
