import re

file_path = r"d:\HB Server\Helbreath-Heldenian-Project-Development\Sources\Server\EnchantingManager.cpp"

with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# Replace OnWithdrawMaterial
old_withdraw = """bool EnchantingManager::OnWithdrawMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;
    
    // We do not have Item IDs for Shards/Fragments yet.
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "The Withdraw function is not enabled yet as Item IDs are missing.");
    return false;
}"""

new_withdraw = """bool EnchantingManager::OnWithdrawMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level)
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
    int item_id = (material_type == 0) ? (5000 + (current_level * 20) + stat_id) : (6000 + (current_level * 50) + stat_id);
    
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
}"""

content = content.replace(old_withdraw, new_withdraw)

# Replace OnDepositMaterials
old_deposit = """bool EnchantingManager::OnDepositMaterials(int client_h, CGame* game, int inventory_slot)
{
    // AÃºn no implementado
    return false;
}"""

new_deposit = """bool EnchantingManager::OnDepositMaterials(int client_h, CGame* game, int inventory_slot)
{
    CClient* client = game->m_client_list[client_h];
    if (!client) return false;

    if (inventory_slot < 0 || inventory_slot >= hb::shared::limits::MaxItems) return false;

    CItem* item = client->m_item_list[inventory_slot];
    if (!item) return false;

    auto config = game->get_item_config(item->m_id_num);
    if (!config || config->m_item_effect_type != 200) {
        game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, "This is not an enchanting material.");
        return false;
    }

    int material_type = config->m_item_effect_value1; // 0 for Shard, 1 for Fragment
    int stat_id = config->m_item_effect_value2;
    int level = config->m_item_effect_value3;
    int amount = item->m_instance.count;

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

    delete client->m_item_list[inventory_slot];
    client->m_item_list[inventory_slot] = nullptr;
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::ItemDepletedEraseItem, inventory_slot, 0, 0, 0);

    sqlite3* db = nullptr;
    std::string dbPath;
    if (EnsureAccountDatabase(client->m_account_name, &db, dbPath)) {
        InsertCharacterEnchantBag(db, client->m_char_name, client->m_enchant_bag);
        CloseAccountDatabase(db);
    }
    SyncEnchantBag(client_h, client);
    
    std::string msg = std::format("Deposited {} {} {}(s).", amount, (material_type == 0 ? GetPrefixName(stat_id) : GetSecondaryName(stat_id)), (material_type == 0 ? "Shard" : "Fragment"));
    game->send_notify_msg(0, client_h, hb::shared::net::Notify::NoticeMsg, 0, 0, 0, msg.c_str());
    return true;
}"""

content = content.replace(old_deposit, new_deposit)

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)

print("Updated EnchantingManager.cpp")
