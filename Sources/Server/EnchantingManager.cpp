// EnchantingManager.cpp
#include "EnchantingManager.h"
#include "Game.h"
#include "Client.h"
#include "Item/Item.h"
#include "ItemManager.h"
#include "AccountSqliteStore.h"
#include "NetMessages.h"
#include "Packet/PacketNotify.h"
#include <algorithm>
#include "SkillManager.h"

extern hb::shared::net::IOServicePool* G_pIOPool;

void EnchantingManager::SyncEnchantBag(int client_h, CClient* client)
{
    if (!client || !client->m_socket) return;

    hb::net::PacketNotifyEnchantBag packet;
    packet.header.msg_id = hb::shared::net::MsgId::Notify;
    packet.header.msg_type = hb::shared::net::Notify::EnchantBagSync;
    packet.count = std::min<uint16_t>(50, static_cast<uint16_t>(client->m_enchant_bag.size()));

    for (int i = 0; i < packet.count; ++i) {
        packet.entries[i].material_type = client->m_enchant_bag[i].material_type;
        packet.entries[i].stat_id = client->m_enchant_bag[i].stat_id;
        packet.entries[i].level = client->m_enchant_bag[i].level;
        packet.entries[i].amount = client->m_enchant_bag[i].amount;
    }

    client->m_socket->send_msg((char*)&packet, sizeof(packet.header) + sizeof(uint16_t) + (sizeof(hb::net::EnchantBagEntry) * packet.count));
}

void EnchantingManager::SyncRecoverQueue(int client_h, CClient* client)
{
    if (!client || !client->m_socket) return;

    hb::net::PacketNotifyRecoverQueue packet;
    packet.header.msg_id = hb::shared::net::MsgId::Notify;
    packet.header.msg_type = hb::shared::net::Notify::RecoverQueueSync;
    packet.count = std::min<uint16_t>(20, static_cast<uint16_t>(client->m_recover_queue.size()));

    for (int i = 0; i < packet.count; ++i) {
        packet.entries[i].db_id = client->m_recover_queue[i].db_id;
        packet.entries[i].item_id = client->m_recover_queue[i].item_id;
        packet.entries[i].item_color = client->m_recover_queue[i].item_color;
        packet.entries[i].prefix_type = client->m_recover_queue[i].prefix_type;
        packet.entries[i].prefix_value = client->m_recover_queue[i].prefix_value;
        packet.entries[i].secondary_type = client->m_recover_queue[i].secondary_type;
        packet.entries[i].secondary_value = client->m_recover_queue[i].secondary_value;
        packet.entries[i].enchant_bonus = client->m_recover_queue[i].enchant_bonus;
        packet.entries[i].spec_value1 = client->m_recover_queue[i].spec_effect_value1;
        packet.entries[i].spec_value2 = client->m_recover_queue[i].spec_effect_value2;
        packet.entries[i].spec_value3 = client->m_recover_queue[i].spec_effect_value3;
        packet.entries[i].shards_yield = client->m_recover_queue[i].shards_yield;
        packet.entries[i].fragments_yield = client->m_recover_queue[i].fragments_yield;
    }

    client->m_socket->send_msg((char*)&packet, sizeof(packet.header) + sizeof(uint16_t) + (sizeof(hb::net::RecoverQueueEntry) * packet.count));
}

std::string GetPrefixName(int prefix) {
    switch (prefix) {
        case 1: return "Critical";
        case 2: return "Poisoning";
        case 3: return "Righteous";
        case 5: return "Agile";
        case 6: return "Light";
        case 7: return "Sharp";
        case 8: return "Strong";
        case 9: return "Ancient";
        case 10: return "Special";
        case 11: return "ManaConverting";
        case 12: return "CritChance";
        default: return "";
    }
}

std::string GetSecondaryName(int sec) {
    switch (sec) {
        case 1: return "Poison Res.";
        case 2: return "Hit Ratio";
        case 3: return "Def Ratio";
        case 4: return "HPRec";
        case 5: return "SPRec";
        case 6: return "MPRec";
        case 7: return "MR";
        case 8: return "PA";
        case 9: return "MA";
        case 10: return "Consecutive Attack";
        case 11: return "Exp Bonus";
        case 12: return "Gold Bonus";
        case 40: return "Hit Ratio %";
        default: return "";
    }
}

int EnchantingManager::GetDisenchantShardYield(CItem* item)
{
    if (!item) return 0;
    if (item->m_instance.prefix_type > 0) return std::max<int>(1, item->m_instance.prefix_value);
    if (item->m_instance.item_color != 0) return 2; // Default for colored items
    return 0;
}

int EnchantingManager::GetDisenchantFragmentYield(CItem* item)
{
    if (!item) return 0;
    if (item->m_instance.secondary_type > 0) return std::max<int>(1, item->m_instance.secondary_value);
    if (item->m_instance.item_color != 0) return 3; // Default for colored items
    return 0;
}

int EnchantingManager::CalculateEnchantSuccessChance(int skill_mastery)
{
    // MaestrÃ­a va de 0 a 100%. Probabilidad base 30% + (0 a 60% por maestrÃ­a)
    // Ejemplo de fÃ³rmula
    int base_chance = 30;
    int mastery_bonus = (skill_mastery * 60) / 100;
    return base_chance + mastery_bonus;
}

bool EnchantingManager::OnDisenchantItem(int client_h, CGame* game, int inventory_slot)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    if (inventory_slot < 0 || inventory_slot >= hb::shared::limits::MaxItems) return false;

    CItem* item = client->m_item_list[inventory_slot];
    if (!item) return false;

    // TODO: Comprobar si estÃ¡ equipado. Si lo estÃ¡, cancelarlo.
    if (client->m_is_item_equipped[inventory_slot]) {
        return false;
    }

    int shards = GetDisenchantShardYield(item);
    int fragments = GetDisenchantFragmentYield(item);

    if (shards == 0 && fragments == 0) {
        // No se puede desencantar objetos sin estadÃ­sticas mÃ¡gicas
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You cannot disenchant this item.");
        return false;
    }

    // AÃ±adir a Recover Queue
    AccountDbRecoverQueueRow rq;
    rq.db_id = 0; // Base de datos lo autoincrementarÃ¡ si lo guardamos (o usar contador)
    rq.item_id = item->m_id_num;
    rq.item_color = item->m_instance.item_color;
    rq.spec_effect_value1 = item->m_instance.special_effect_value1;
    rq.spec_effect_value2 = item->m_instance.special_effect_value2;
    rq.spec_effect_value3 = item->m_instance.special_effect_value3;
    rq.prefix_type = item->m_instance.prefix_type;
    rq.prefix_value = item->m_instance.prefix_value;
    rq.secondary_type = item->m_instance.secondary_type;
    rq.secondary_value = item->m_instance.secondary_value;
    rq.enchant_bonus = item->m_instance.enchant_bonus;
    rq.shards_yield = shards;
    rq.fragments_yield = fragments;

    client->m_recover_queue.insert(client->m_recover_queue.begin(), rq);
    if (client->m_recover_queue.size() > 10) {
        client->m_recover_queue.pop_back();
    }

    // AÃ±adir Shards y Fragmentos a la bolsa (BÃºsqueda o creaciÃ³n)
    auto AddMaterial = [&](int type, int stat_id, int level, int amount) {
        if (amount <= 0) return;
        bool found = false;
        for (auto& bagRow : client->m_enchant_bag) {
            if (bagRow.material_type == type && bagRow.stat_id == stat_id && bagRow.level == level) {
                bagRow.amount += amount;
                found = true;
                break;
            }
        }
        if (!found) {
            AccountDbEnchantBagRow bRow = { type, stat_id, level, amount };
            client->m_enchant_bag.push_back(bRow);
        }
    };

    AddMaterial(0, item->m_instance.prefix_type, item->m_instance.prefix_value, shards); // 0 = Shard
    AddMaterial(1, item->m_instance.secondary_type, item->m_instance.secondary_value, fragments); // 1 = Fragment

    // Borrar item original
    
    delete client->m_item_list[inventory_slot]; client->m_item_list[inventory_slot] = nullptr;

    // Sincronizar cliente
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::ItemDepletedEraseItem, inventory_slot, 0, 0, 0);
    
    std::string result_msg = "Item successfully disenchanted. ";
    if (shards > 0) {
        std::string stat_name = GetPrefixName(item->m_instance.prefix_type);
        if (stat_name.empty() && item->m_instance.item_color != 0) stat_name = "Color";
        result_msg += std::format("Gained {} {} Shard(s). ", shards, stat_name);
    }
    if (fragments > 0) {
        std::string stat_name = GetSecondaryName(item->m_instance.secondary_type);
        if (stat_name.empty() && item->m_instance.item_color != 0) stat_name = "Color";
        result_msg += std::format("Gained {} {} Fragment(s).", fragments, stat_name);
    }
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, result_msg.c_str());

    // Otorgar EXP de Enchanting
    {
        if (client->m_skill_mastery[24] == 0) {
            client->m_skill_mastery[24] = 1;
            game->send_notify_msg(0, client_h, hb::shared::net::Notify::Skill, 24, 1, 0, 0);
        }
        int max_level = std::max((int)item->m_instance.prefix_value, (int)item->m_instance.secondary_value);
        int chance = 10 + (max_level * 5); // Level 1 = 15%, Level 15 = 85%
        if (std::rand() % 100 < chance) {
            game->m_skill_manager->calculate_ssn_skill_index(client_h, 24, 1);
        }
    }
    SyncEnchantBag(client_h, client);
    SyncRecoverQueue(client_h, client);
    return true;
}

bool EnchantingManager::OnEnchantItem(int client_h, CGame* game, int inventory_slot, int target_stat_id)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    if (inventory_slot < 0 || inventory_slot >= hb::shared::limits::MaxItems) return false;

    CItem* item = client->m_item_list[inventory_slot];
    if (!item) return false;

    if (client->m_is_item_equipped[inventory_slot]) return false;

    bool item_changed = false;
    bool bag_changed = false;

    // Try to enchant prefix (material_type = 0)
    if (target_stat_id == 0 && item->m_instance.prefix_type > 0 && item->m_instance.prefix_value < 15) {
        auto it = std::find_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [&](const AccountDbEnchantBagRow& r) {
            return r.material_type == 0 && r.stat_id == item->m_instance.prefix_type && r.level == item->m_instance.prefix_value && r.amount > 0;
        });
        if (it != client->m_enchant_bag.end()) {
            it->amount--;
            if (it->amount <= 0) client->m_enchant_bag.erase(it);
            item->m_instance.prefix_value++;
            item_changed = true;
            bag_changed = true;

            std::string msg = std::format("Primary attribute {} successfully enchanted to +{}.", GetPrefixName(item->m_instance.prefix_type), item->m_instance.prefix_value);
            game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
            
            int chance = 10 + (item->m_instance.prefix_value * 5);
            if (std::rand() % 100 < chance) {
                if (client->m_skill_mastery[24] == 0) {
                    client->m_skill_mastery[24] = 1;
                    game->send_notify_msg(0, client_h, hb::shared::net::Notify::Skill, 24, 1, 0, 0);
                }
                game->m_skill_manager->calculate_ssn_skill_index(client_h, 24, 1);
            }
            
            goto ENCHANT_DONE;
        }
    }

    // Try to enchant secondary (material_type = 1)
    if (target_stat_id == 1 && item->m_instance.secondary_type > 0 && item->m_instance.secondary_value < 15) {
        auto it = std::find_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [&](const AccountDbEnchantBagRow& r) {
            return r.material_type == 1 && r.stat_id == item->m_instance.secondary_type && r.level == item->m_instance.secondary_value && r.amount > 0;
        });
        if (it != client->m_enchant_bag.end()) {
            it->amount--;
            if (it->amount <= 0) client->m_enchant_bag.erase(it);
            item->m_instance.secondary_value++;
            item_changed = true;
            bag_changed = true;

            std::string msg = std::format("Secondary attribute {} successfully enchanted to +{}.", GetSecondaryName(item->m_instance.secondary_type), item->m_instance.secondary_value);
            game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());

            int chance = 10 + (item->m_instance.secondary_value * 5);
            if (std::rand() % 100 < chance) {
                if (client->m_skill_mastery[24] == 0) {
                    client->m_skill_mastery[24] = 1;
                    game->send_notify_msg(0, client_h, hb::shared::net::Notify::Skill, 24, 1, 0, 0);
                }
                game->m_skill_manager->calculate_ssn_skill_index(client_h, 24, 1);
            }

            goto ENCHANT_DONE;
        }
    }

    // If we reach here, we didn't have materials
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You do not have the proper level of ingredients to enchant this item.");
    return false;

ENCHANT_DONE:
    if (item_changed) {
        game->send_item_attribute_change(client_h, inventory_slot, item);
    }
    if (bag_changed) {
        sqlite3* db = nullptr;
        std::string dbPath;
        if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
            InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
            CloseAccountDatabase(db);
        }
        SyncEnchantBag(client_h, client);
    }

    return true;
}

bool EnchantingManager::OnRecoverItem(int client_h, CGame* game, int queue_db_id)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    // Buscar en la cola
    auto it = std::find_if(client->m_recover_queue.begin(), client->m_recover_queue.end(), 
        [queue_db_id](const AccountDbRecoverQueueRow& row) { return static_cast<int>(row.db_id) == queue_db_id; });

    if (it == client->m_recover_queue.end()) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Item not found in recovery queue.");
        return false;
    }

    AccountDbRecoverQueueRow rq = *it;

    // Check if player has the required shards and fragments
    bool has_shards = (rq.shards_yield == 0);
    bool has_fragments = (rq.fragments_yield == 0);
    
    int shard_idx = -1;
    int frag_idx = -1;

    for (size_t i = 0; i < client->m_enchant_bag.size(); ++i) {
        if (!has_shards && client->m_enchant_bag[i].material_type == 0 && client->m_enchant_bag[i].stat_id == rq.prefix_type && client->m_enchant_bag[i].level == rq.prefix_value) {
            if (client->m_enchant_bag[i].amount >= rq.shards_yield) {
                has_shards = true;
                shard_idx = static_cast<int>(i);
            }
        }
        if (!has_fragments && client->m_enchant_bag[i].material_type == 1 && client->m_enchant_bag[i].stat_id == rq.secondary_type && client->m_enchant_bag[i].level == rq.secondary_value) {
            if (client->m_enchant_bag[i].amount >= rq.fragments_yield) {
                has_fragments = true;
                frag_idx = static_cast<int>(i);
            }
        }
    }

    if (!has_shards || !has_fragments) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Not enough shards or fragments to recover this item.");
        return false;
    }
    
    // Find empty inventory slot
    int empty_slot = -1;
    for (int i = 0; i < hb::shared::limits::MaxItems; ++i) {
        if (client->m_item_list[i] == nullptr) {
            empty_slot = i;
            break;
        }
    }
    
    if (empty_slot == -1) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Your inventory is full. Please free up space to recover the item.");
        return false;
    }

    // Deduct materials
    if (shard_idx != -1) {
        client->m_enchant_bag[shard_idx].amount -= rq.shards_yield;
    }
    if (frag_idx != -1) {
        client->m_enchant_bag[frag_idx].amount -= rq.fragments_yield;
    }
    
    // Clean up empty bag slots
    client->m_enchant_bag.erase(std::remove_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [](const AccountDbEnchantBagRow& r) { return r.amount <= 0; }), client->m_enchant_bag.end());

    // Remove from recover queue
    client->m_recover_queue.erase(it);

    // Recreate the item
    CItem* recovered_item = new CItem;
    if (!game->m_item_manager->init_item_attr(recovered_item, rq.item_id)) {
        delete recovered_item;
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Error recreating the item.");
        return false;
    }
    
    recovered_item->m_instance.item_color = rq.item_color;
    recovered_item->m_instance.prefix_type = rq.prefix_type;
    recovered_item->m_instance.prefix_value = rq.prefix_value;
    recovered_item->m_instance.secondary_type = rq.secondary_type;
    recovered_item->m_instance.secondary_value = rq.secondary_value;
    recovered_item->m_instance.enchant_bonus = rq.enchant_bonus;
    recovered_item->m_instance.special_effect_value1 = rq.spec_effect_value1;
    recovered_item->m_instance.special_effect_value2 = rq.spec_effect_value2;
    recovered_item->m_instance.special_effect_value3 = rq.spec_effect_value3;
    
    client->m_item_list[empty_slot] = recovered_item;

    // Save state
    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
        InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
        InsertCharacterRecoverQueue(db, client->m_char_name, client->m_recover_queue);
        CloseAccountDatabase(db);
    }
    
    // Sync with client
    SyncEnchantBag(client_h, client);
    SyncRecoverQueue(client_h, client);
    
    game->m_item_manager->send_item_notify_msg(client_h, hb::shared::net::Notify::ItemObtained, recovered_item, 0);
    game->send_item_attribute_change(client_h, empty_slot, recovered_item);
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Item successfully recovered.");

    return true;
}

bool EnchantingManager::OnDepositMaterials(int client_h, CGame* game, int inventory_slot)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    int deposited_count = 0;
    bool db_needs_update = false;

    // Helper lambda to deposit a single slot
    auto DepositSlot = [&](int slot) {
        if (slot < 0 || slot >= hb::shared::limits::MaxItems) return false;
        CItem* item = client->m_item_list[slot];
        if (!item) return false;

        int id = item->m_id_num;
        if (id < 0 || id >= hb::server::config::MaxItemTypes) return false;
        CItem* config = game->m_item_config_list[id];
        if (!config || config->m_item_effect_type != 200) {
            return false;
        }

        int material_type = config->m_item_effect_value1; // 0 for Shard, 1 for Fragment
        int stat_id = config->m_item_effect_value2;
        int level = config->m_item_effect_value3;
        int amount = static_cast<int>(item->m_instance.count);
        if (amount <= 0) amount = 1;

        bool found = false;
        for (auto& bagRow : client->m_enchant_bag) {
            if (bagRow.material_type == material_type && bagRow.stat_id == stat_id && bagRow.level == level) {
                bagRow.amount += amount;
                found = true;
                break;
            }
        }
        if (!found) {
            AccountDbEnchantBagRow bRow = { material_type, stat_id, level, amount };
            client->m_enchant_bag.push_back(bRow);
        }

        delete client->m_item_list[slot];
        client->m_item_list[slot] = nullptr;
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::ItemDepletedEraseItem, slot, 0, 0, 0);
        
        deposited_count += amount;
        db_needs_update = true;
        return true;
    };

    if (inventory_slot == -1) {
        // Deposit All
        for (int i = 0; i < hb::shared::limits::MaxItems; ++i) {
            DepositSlot(i);
        }
        if (deposited_count == 0) {
            game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "No enchanting materials found in inventory.");
            return false;
        }
    } else {
        // Deposit single slot
        if (!DepositSlot(inventory_slot)) {
            game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "This is not an enchanting material.");
            return false;
        }
    }

    if (db_needs_update) {
        sqlite3* db = nullptr;
        std::string dbPath;
        if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
            InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
            CloseAccountDatabase(db);
        }
        SyncEnchantBag(client_h, client);
        
        std::string msg = std::format("Deposited a total of {} material(s).", deposited_count);
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
    }
    
    return true;
}













int EnchantingManager::GetRequiredLevelForUpgrade(int value)
{
    if (value >= 1 && value <= 5)
    {
        return 4;
    }
    else if (value > 5 && value <= 10)
    {
        return 3;
    }

    return 2;
}

bool EnchantingManager::OnUpgradeMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    if (current_level < 1 || current_level >= 17) return false;

    int required_amount = GetRequiredLevelForUpgrade(current_level);

    // Buscar material actual
    auto it_current = std::find_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [&](const AccountDbEnchantBagRow& r) {
        return r.material_type == material_type && r.stat_id == stat_id && r.level == current_level;
    });

    if (it_current == client->m_enchant_bag.end() || it_current->amount < required_amount) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You do not have enough ingredients to upgrade.");
        return false;
    }

    // Descontar
    it_current->amount -= required_amount;
    
    if (it_current->amount <= 0) {
        client->m_enchant_bag.erase(it_current);
    }

    // Añadir nivel + 1
    int next_level = current_level + 1;
    bool found_next = false;
    for (auto& row : client->m_enchant_bag) {
        if (row.material_type == material_type && row.stat_id == stat_id && row.level == next_level) {
            row.amount += 1;
            found_next = true;
            break;
        }
    }

    if (!found_next) {
        AccountDbEnchantBagRow bRow = { material_type, stat_id, next_level, 1 };
        client->m_enchant_bag.push_back(bRow);
    }

    // Actualizar Base de Datos (Account DB)
    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
        InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
        CloseAccountDatabase(db);
    }

    // Sincronizar UI del Cliente
    SyncEnchantBag(client_h, client);
    std::string upg_msg = std::format("{} {}(s) upgraded successfully.", required_amount, stat_id < 20 ? GetPrefixName(stat_id) : GetSecondaryName(stat_id)); // Basic approximation for stat_id
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, upg_msg.c_str());
    
    return true;
}

bool EnchantingManager::OnUpgradeMaterialAll(int client_h, CGame* game, int material_type, int stat_id, int current_level)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    if (current_level < 1 || current_level >= 17) return false;

    int required_amount = GetRequiredLevelForUpgrade(current_level);

    auto it_current = std::find_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [&](const AccountDbEnchantBagRow& r) {
        return r.material_type == material_type && r.stat_id == stat_id && r.level == current_level;
    });

    if (it_current == client->m_enchant_bag.end() || it_current->amount < required_amount) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "You do not have enough ingredients to upgrade.");
        return false;
    }

    int possible_upgrades = it_current->amount / required_amount;
    if (possible_upgrades <= 0) return false;

    it_current->amount -= (possible_upgrades * required_amount);
    if (it_current->amount <= 0) {
        client->m_enchant_bag.erase(it_current);
    }

    int next_level = current_level + 1;
    bool found_next = false;
    for (auto& row : client->m_enchant_bag) {
        if (row.material_type == material_type && row.stat_id == stat_id && row.level == next_level) {
            row.amount += possible_upgrades;
            found_next = true;
            break;
        }
    }

    if (!found_next) {
        AccountDbEnchantBagRow bRow = { material_type, stat_id, next_level, possible_upgrades };
        client->m_enchant_bag.push_back(bRow);
    }

    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
        InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
        CloseAccountDatabase(db);
    }

    SyncEnchantBag(client_h, client);
    return true;
}

bool EnchantingManager::OnWithdrawMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    auto it = std::find_if(client->m_enchant_bag.begin(), client->m_enchant_bag.end(), [&](const AccountDbEnchantBagRow& r) {
        return r.material_type == material_type && r.stat_id == stat_id && r.level == current_level && r.amount > 0;
    });

    if (it == client->m_enchant_bag.end()) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Material not found in bag.");
        return false;
    }

    int amount_to_withdraw = it->amount;
    int item_id = (material_type == 0) ? (1200 + ((current_level - 1) * 20) + stat_id) : (2000 + ((current_level - 1) * 50) + stat_id);
    
    CItem* item = new CItem;
    if (!game->m_item_manager->init_item_attr(item, item_id)) {
        delete item;
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Error generating physical material.");
        return false;
    }
    
    item->m_instance.count = amount_to_withdraw;

    int del_req = 0;
    if (!game->m_item_manager->add_client_item_list(client_h, item, &del_req)) {
        delete item;
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "Your inventory is full or too heavy.");
        return false;
    }

    if (del_req == 0) {
        game->m_item_manager->send_item_notify_msg(client_h, hb::shared::net::Notify::ItemObtained, item, 0);
    } else {
        for (int i = 0; i < hb::shared::limits::MaxItems; ++i) {
            if (client->m_item_list[i] && client->m_item_list[i]->m_id_num == item_id) {
                game->send_item_attribute_change(client_h, i, client->m_item_list[i]);
                break;
            }
        }
        delete item;
    }

    client->m_enchant_bag.erase(it);

    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
        InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
        CloseAccountDatabase(db);
    }
    SyncEnchantBag(client_h, client);
    
    std::string msg = std::format("Withdrawn {} {} {}(s).", amount_to_withdraw, (material_type == 0 ? GetPrefixName(stat_id) : GetSecondaryName(stat_id)), (material_type == 0 ? "Shard" : "Fragment"));
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
    return true;
}
