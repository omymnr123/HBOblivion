#pragma once

#include "PacketHeaders.h"
#include "Appearance.h"
#include "NetConstants.h"

#include <cstdint>

namespace hb {
namespace net {
	HB_PACK_BEGIN
	struct HB_PACKED PacketLogCharacterListHeader {
		PacketHeader header;
		std::int32_t total_chars;
	};

	struct HB_PACKED PacketLogCharacterEntry {
    PacketHeader header;
    char name[20];
    hb::shared::entity::PlayerAppearance appearance;
    uint8_t sex;
    uint8_t skin;
    int32_t level;
    int32_t exp;
    int32_t prestige_level; // <--- AÑADE ESTA LÍNEA AQUÍ
    char map_name[20];
};

	struct HB_PACKED PacketLogNewCharacterCreatedHeader {
		PacketHeader header;
		char character_name[hb::shared::limits::CharNameLen];
		std::int32_t total_chars;
	};

	struct HB_PACKED PacketLogResponseReject {
		PacketHeader header;
		std::int32_t block_year;
		std::int32_t block_month;
		std::int32_t block_day;
	};

	struct HB_PACKED PacketLogEnterGameConfirm {
		PacketHeader header;
		char game_server_addr[16];
		std::uint16_t game_server_port;
		char game_server_name[20];
	};

	struct HB_PACKED PacketLogResponseCode {
		PacketHeader header;
		std::uint8_t code;
	};
	HB_PACK_END
}
}
