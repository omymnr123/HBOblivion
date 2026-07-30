#include "CombatSystem.h"
#include "Player.h"
#include "Game.h"
#include "Item/Item.h"

using namespace hb::shared::item;

combat_system& combat_system::get()
{
	static combat_system instance;
	return instance;
}

void combat_system::set_player(CPlayer& player)
{
	m_player = &player;
}

void combat_system::set_game(CGame& game)
{
	m_game = &game;
}

// Returns the weapon_class enum for the currently equipped weapon.
// Returns weapon_class::none (0) if unarmed or config unavailable.
int8_t combat_system::get_weapon_class() const
{
	if (!m_player || !m_game) return weapon_class::none;
	int16_t weapon_id = m_player->m_playerAppearance.weapon_item_id;
	if (weapon_id <= 0) return weapon_class::none;
	CItem* cfg = m_game->get_item_config(weapon_id);
	if (!cfg) return weapon_class::none;
	return cfg->get_weapon_class();
}

int combat_system::get_attack_type() const
{
	if (!m_player) return 0;
	int8_t wc = get_weapon_class();
	int skill_idx = get_weapon_skill_type(); // <-- Identificamos el skill del arma equipada

	// --- BLOQUEO VISUAL: Solo es "super" si hay cargas, está activo Y el skill es 100 ---
	bool super = (m_player->m_super_attack_left > 0) && 
	             (m_player->m_super_attack_mode == true) && 
	             (m_player->m_skill_mastery[skill_idx] >= 100);

	switch (wc) {
	case weapon_class::none:
		if (super) return 20;
		return 1; // Unarmed
	case weapon_class::dagger:
	case weapon_class::short_sword:
		if (super) return 21;
		return 1; // Short sword
	case weapon_class::fencing:
		if (super) return 22;
		return 1; // Fencing
	case weapon_class::long_sword:
	{
		// StormBringer (item 845) has unique super attack animation
		int16_t weapon_id = m_player->m_playerAppearance.weapon_item_id;
		if (weapon_id == 845)
		{
			if (super) return 30;
			return 5; // StormBringer normal attack
		}
		if (super) return 23;
		return 1; // Long sword
	}
	case weapon_class::axe:
		if (super) return 24;
		return 1; // Axe
	case weapon_class::hammer:
		if (super) return 26;
		return 1; // Hammer
	case weapon_class::wand:
		if (super) return 27;
		return 1; // Wand
	case weapon_class::bow:
		if (super) return 25;
		return 2; // Bow
	}
	return 0;
}

int combat_system::get_weapon_skill_type() const
{
	if (!m_player) return 1;
	switch (get_weapon_class()) {
	case weapon_class::none:        return 5;  // Openhand
	case weapon_class::dagger:
	case weapon_class::short_sword: return 7;  // Short Sword
	case weapon_class::long_sword:  return 8;  // Long Sword
	case weapon_class::fencing:     return 9;  // Fencing
	case weapon_class::axe:         return 10; // Axe
	case weapon_class::hammer:      return 14; // Hammer
	case weapon_class::wand:        return 21; // Wand
	case weapon_class::bow:         return 6;  // Bow
	}
	return 1; // Fishing
}

bool combat_system::can_super_attack() const
{
	if (!m_player) return false;
	int skill_idx = get_weapon_skill_type(); // <-- Identificamos el skill del arma equipada

	// --- BLOQUEO UI: Retorna falso si el skill no está al 100 ---
	return m_player->m_super_attack_left > 0
		&& m_player->m_super_attack_mode
		&& m_player->m_skill_mastery[skill_idx] >= 100;
}