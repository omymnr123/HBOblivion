#pragma once

#include "PacketHeaders.h"
#include <cstdint>

namespace hb::shared::net
{

HB_PACK_BEGIN

// Base structure for GuildSystem packets
// Removed because inheriting non-static members breaks std::is_standard_layout,
// causing hb::net::PacketCast to silently drop all packets.

// Represents a single member in the UI list
struct HB_PACKED GuildMemberInfo
{
	char name[12];
	std::uint8_t rank;
	bool is_online;
	char extra_info[20];
};

// Response from server with the member list and current tokens
struct HB_PACKED PacketGuildMemberList : public hb::net::packet_base
{
	hb::net::PacketHeader header;
	std::uint32_t msg_size;
	
	std::uint32_t tokens;
	std::uint8_t guild_level;
	std::uint32_t guild_gxp;
	std::uint8_t skills[4]; // Indices 0 to 3 match SkillId 1 to 4
	std::uint16_t member_count;
	GuildMemberInfo members[100]; // GameConstants has max_guild_names = 100
};

// Action request from client (promote, demote, disband, donate, buy)
struct HB_PACKED PacketGuildAction : public hb::net::packet_base
{
	hb::net::PacketHeader header;
	std::uint32_t msg_size;
	
	char target_name[12]; // For promote/demote
	std::uint32_t amount; // For donate
	char item_name[20];   // For shop buy
};

struct HB_PACKED PacketGuildNotifyInvite : public hb::net::packet_base
{
	hb::net::PacketHeader header;
	std::uint32_t msg_size;
	char inviter_name[12];
	char guild_name[20];
};

HB_PACK_END

} // namespace hb::shared::net
