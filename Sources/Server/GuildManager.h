#pragma once

#include <string>
#include <map>
#include <cstdint>

class CGame;
struct sqlite3;

enum class GuildRank {
	Master = 1,
	Officer = 2,
	Member = 3
};

enum class GuildSkillId {
	Guerrero = 1, // Physical Damage & HP
	Mago = 2,     // Magic Res & MP
	Recolector = 3, // Mining/Fishing
	Cazador = 4   // Drop Rate
};

struct GuildConfig {
	static constexpr int MAX_GUILD_LEVEL = 20;
	static constexpr int MAX_SKILL_LEVEL = 5;
	
	static constexpr int GOLD_PER_GXP = 10000; // 1 GXP per 10k gold donated
	
	// Bonus formulas per skill level
	static constexpr int BONUS_HP_PER_LEVEL = 50;
	static constexpr int BONUS_PHYS_DMG_PERCENT = 2;
	
	static constexpr int BONUS_MP_PER_LEVEL = 50;
	static constexpr int BONUS_MAGIC_DMG_PERCENT = 2;
	
	static constexpr int BONUS_GATHER_PERCENT = 10;
	static constexpr int BONUS_DROP_RATE_PERCENT = 500;

	// GXP required for each level (index = level. level 0 is not used. level 1 = 0 gxp)
	static int get_gxp_requirement(int level) {
		if (level <= 1) return 0;
		// e.g. Lvl 2: 100 GXP (1M gold)
		// Lvl 3: 300 GXP (3M gold)
		// Lvl 4: 600 GXP (6M gold)
		// Lvl n: 100 * (level-1) * level / 2
		return 100 * (level - 1) * level / 2;
	}
};

class GuildManager
{
public:
	GuildManager() = default;
	~GuildManager() = default;

	void set_game(CGame* game) { m_game = game; }
	bool initialize();
	void cleanup();

	// Guild commands
	void handle_create_guild(int client_h, const char* guild_name);
	void handle_disband_guild(int client_h);
	void update_member_login(const char* char_name, uint32_t timestamp);
	void create_guild(int client_h, const std::string& guild_name);
	void invite_member(int client_h, const std::string& target_name);
	void accept_invite(int client_h);
	void kick_member(int client_h, const std::string& target_name);
	void leave_guild(int client_h);

	// Utilities
	void broadcast_guild_chat(int client_h, const char* message);
	void get_player_guild_info(const std::string& char_name, uint32_t& out_guid, int& out_rank);
	sqlite3* get_db() const { return m_db; }

	// Phase 2: Progression & Economy
	void add_guild_gxp(uint32_t guild_guid, int amount);
	void add_member_tokens(int client_h, int amount);
	void process_donate_command(int client_h, int gold_amount);
	void show_guild_info(int client_h);
	void process_shop_command(int client_h, const std::string& item_name);

	// Management
	void promote_member(int client_h, const std::string& target_name);
	void demote_member(int client_h, const std::string& target_name);
	void disband_guild(int client_h);
	void list_members(int client_h);
	void send_guild_info_to_client(int client_h);

	// Phase 4: Progression & Skills
	int get_guild_level(uint32_t guild_guid);
	int get_guild_skill_level(uint32_t guild_guid, int skill_id);
	int get_player_guild_skill(int client_h, int skill_id);
	void process_upgrade_skill_command(int client_h, int skill_id);
	void broadcast_guild_info_update(uint32_t guild_guid);
	
	// --- NUEVO: Funcion para actualizar cada minuto ---
	void update(uint32_t current_time);

private:
	CGame* m_game = nullptr;
	sqlite3* m_db = nullptr;

	uint32_t m_last_broadcast_time = 0; // --- NUEVO: Temporizador ---

	// client_h -> invited_by_client_h
	std::map<int, int> m_pending_invites;

	// Cache of guild skills: guild_guid -> (skill_id -> level)
	std::map<uint32_t, std::map<int, int>> m_guild_skills_cache;
	void load_guild_skills_cache();

	// Helper: broadcast NullAction to update guild name visually for nearby players
	void broadcast_guild_visual_update(int client_h);
};