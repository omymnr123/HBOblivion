#include "GameCmdReroll.h"
#include "Game.h"
#include "Client.h"
#include "ItemManager.h"
#include "Npc.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdio>

using namespace hb::shared::net;
using namespace hb::shared::item;

bool GameCmdReroll::execute(CGame* game, int client_h, const char* args)
{
	if (game == nullptr || game->m_client_list[client_h] == nullptr)
		return false;

	CClient* client = game->m_client_list[client_h];
	if (!client->m_is_init_complete)
		return false;

	// Comprobar que estas cerca del NPC Herrero
	bool is_near_blacksmith = false;
	for (int i = 1; i < 10000; i++) { // <-- Bucle forzado a 10000 monstruos
		CNpc* npc = game->m_npc_list[i];
		if (npc == nullptr) continue;
		if (npc->m_hp <= 0) continue;
		if (npc->m_map_index != client->m_map_index) continue;

		int dist_x = std::abs(npc->m_x - client->m_x);
		int dist_y = std::abs(npc->m_y - client->m_y);

		if (dist_x <= 6 && dist_y <= 6) {
			if (npc->m_type == 24 ||
				std::strstr(npc->m_npc_name, "BS") != nullptr ||
				std::strstr(npc->m_npc_name, "Blacksmith") != nullptr ||
				std::strstr(npc->m_npc_name, "Herrero") != nullptr)
			{
				is_near_blacksmith = true;
				break;
			}
		}
	}

	if (!is_near_blacksmith && client->m_admin_level == 0) {
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0,
			"Debes estar cerca del NPC Herrero (Blacksmith) para rerollear stats.");
		return true;
	}

	// Saltar espacios
	while (args != nullptr && (*args == ' ' || *args == '\t'))
		args++;

	// Si no se pone slot, muestra la ayuda y los items que se pueden cambiar
	if (args == nullptr || *args == '\0') {
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0,
			"--- NPC Herrero: Reroll de Stats ---");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0,
			"Uso: /reroll <numero_slot> (Costo: 100,000 Oro)");

		int count = 0;
		for (int i = 0; i < 50; i++) { // <-- Bucle forzado a 50 huecos de inventario
			CItem* item = client->m_item_list[i];
			if (item != nullptr) {
				if (item->get_item_effect_type() == ItemEffectType::Attack ||
					item->get_item_effect_type() == ItemEffectType::Defense ||
					item->get_item_effect_type() == ItemEffectType::AttackManaSave)
				{
					count++;
					char text[128];
					snprintf(text, sizeof(text), "Slot %d: [Disponible para Reroll]", i + 1);
					game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, text);
				}
			}
		}

		if (count == 0) {
			game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0,
				"No tienes armaduras ni armas rerolleables en tu inventario.");
		}

		return true;
	}

	int slot = std::atoi(args);
	if (slot < 1 || slot > 50) { // <-- Límite forzado a 50 huecos
		char msg[128];
		snprintf(msg, sizeof(msg), "Numero de slot invalido. Usa un slot entre 1 y 50.");
		game->send_notify_msg(0, client_h, Notify::NoticeMsg, 0, 0, 0, msg);
		return true;
	}

	int item_index = slot - 1;
	game->m_item_manager->reroll_item_attributes(client_h, item_index, 100000);

	return true;
}