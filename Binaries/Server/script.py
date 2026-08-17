import sqlite3

conn = sqlite3.connect('gamedata.db')

# Insert Ares Shield Generator
conn.execute("""
INSERT INTO npc_configs (
    npc_id, name, npc_type, hp_min, hp_max, hold_resist, defense_ratio, hit_ratio, min_bravery,
    exp_min, exp_max, gold_min, gold_max, min_damage, max_damage, npc_size, side, action_limit,
    action_time, resist_magic, magic_level, day_of_week_limit, chat_msg_presence, target_search_range,
    regen_time, attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id
)
SELECT 
    200, 'Ares Shield Generator', npc_type, hp_min, hp_max, hold_resist, defense_ratio, hit_ratio, min_bravery,
    exp_min, exp_max, gold_min, gold_max, min_damage, max_damage, npc_size, side, action_limit,
    action_time, resist_magic, magic_level, day_of_week_limit, chat_msg_presence, target_search_range,
    regen_time, attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id
FROM npc_configs WHERE npc_id = 74
""")

# Insert Elv Shield Generator
conn.execute("""
INSERT INTO npc_configs (
    npc_id, name, npc_type, hp_min, hp_max, hold_resist, defense_ratio, hit_ratio, min_bravery,
    exp_min, exp_max, gold_min, gold_max, min_damage, max_damage, npc_size, side, action_limit,
    action_time, resist_magic, magic_level, day_of_week_limit, chat_msg_presence, target_search_range,
    regen_time, attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id
)
SELECT 
    201, 'Elv Shield Generator', npc_type, hp_min, hp_max, hold_resist, defense_ratio, hit_ratio, min_bravery,
    exp_min, exp_max, gold_min, gold_max, min_damage, max_damage, npc_size, side, action_limit,
    action_time, resist_magic, magic_level, day_of_week_limit, chat_msg_presence, target_search_range,
    regen_time, attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id
FROM npc_configs WHERE npc_id = 75
""")

conn.commit()
print("Successfully inserted rows 200 and 201.")
