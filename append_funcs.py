import io

code = """
CNpc* CEntityManager::get_entity(int entity_handle) const
{
    if (entity_handle < 0 || entity_handle >= hb::server::config::MaxNpcs)
        return nullptr;
        
    return m_npc_list[entity_handle];
}

// ========================================================================
// Ancient Mana Nodes Event
// ========================================================================

void CEntityManager::spawn_mana_node_event()
{
    // 1. Select a random combat map (not cities like aresden or elvine)
    std::vector<int> valid_maps;
    for (int i = 0; i < m_max_maps; i++) {
        if (m_map_list[i] != nullptr) {
            std::string map_name = m_map_list[i]->m_name;
            // Basic blacklist of safe/town maps
            if (map_name.find("aresden") != std::string::npos ||
                map_name.find("elvine") != std::string::npos ||
                map_name.find("arenal") != std::string::npos ||
                map_name.find("jail") != std::string::npos ||
                map_name.find("resurr") != std::string::npos ||
                map_name.find("fight") != std::string::npos ||
                map_name == "gldhall_1" || map_name == "gldhall_2" ||
                map_name == "toh1" || map_name == "toh2" || map_name == "toh3") {
                continue;
            }
            
            // Collect valid maps that have spots configured
            bool has_mobs = false;
            for(int s = 0; s < hb::server::map::MaxSpotMobGenerator; s++) {
                if (m_map_list[i]->m_spot_mob_generator[s].is_defined) {
                    has_mobs = true;
                    break;
                }
            }
            if (has_mobs) {
                valid_maps.push_back(i);
            }
        }
    }

    if (valid_maps.empty()) return;

    int map_idx = valid_maps[rand() % valid_maps.size()];
    CMap* map = m_map_list[map_idx];

    // 2. Pick a random location within the map (avoid edges)
    int spawn_x = 15 + (rand() % (map->m_size_x - 30));
    int spawn_y = 15 + (rand() % (map->m_size_y - 30));

    // Try to find a walkable tile nearby
    bool found_tile = false;
    for(int try_count = 0; try_count < 10; try_count++) {
        if (map->get_moveable(spawn_x, spawn_y)) {
            found_tile = true;
            break;
        }
        spawn_x += (rand() % 5) - 2;
        spawn_y += (rand() % 5) - 2;
    }

    if (!found_tile) return; // Unlucky

    // 3. Instantiate the Mana Node
    // Tiers 1, 2, 3 -> NPC IDs 150, 151, 152
    int tier = (rand() % 100);
    int node_npc_id = 150; // Tier 1
    if (tier > 85) node_npc_id = 152; // 15% Tier 3
    else if (tier > 50) node_npc_id = 151; // 35% Tier 2
    
    char name_buf[20];
    sprintf(name_buf, "_MANANODE%d", rand() % 9999);
    
    int dummy_offset_x = 0, dummy_offset_y = 0;
    
    int node_h = create_entity(
        node_npc_id, name_buf, map->m_name,
        1 /* Class: non-moving */, 0 /* sa */, MoveType::Guard /* MoveType */,
        &dummy_offset_x, &dummy_offset_y,
        nullptr /* waypoints */, nullptr /* area */,
        0, 0 /* side */,
        false /* hide */, false /* summoned */,
        false /* berserk */, false /* master */,
        true /* bypass limit */
    );

    if (node_h > 0) {
        CNpc* node = get_entity(node_h);
        if (node) {
            node->m_x = spawn_x;
            node->m_y = spawn_y;
            node->m_type = 42; // ManaStone
            map->set_owner(node_h, 2 /* NPC */, spawn_x, spawn_y);

            char msg[128];
            sprintf(msg, "--> A new Ancient Mana Node has appeared in %s! <--", map->m_name);
            hb::logger::log("{}", msg);
        }
    }
}
"""

with io.open('Sources/Server/EntityManager.cpp', 'a', encoding='utf-8') as f:
    f.write("\n" + code)
