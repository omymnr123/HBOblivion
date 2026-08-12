-- Migration: insert Haste Manual (StudyMagic for magic_id 78)
-- Safe, idempotent: will not insert if an item with name 'Haste Manual' or item_id 2000 already exists.
BEGIN;

-- Insert a new item row copying attributes from Cancellation Manual (item_id = 852), but change id, name and study target (magic id = 78)
INSERT INTO items (
    item_id,name,item_type,item_sub_type,equip_pos,weapon_class,
    item_effect_type,item_effect_value1,item_effect_value2,item_effect_value3,
    item_effect_value4,item_effect_value5,item_effect_value6,durability,special_effect,
    sell_price,weight,swing_speed,level_requirement,gender_requirement,
    special_effect_value1,special_effect_value2,related_skill,hide_armor,is_skirt,
    stackable,is_dyeable,set_id,item_color,display_id,armor_class,attribute_pool_id
)
SELECT
    2000,                         -- new item_id (change if already used in your environment)
    'Haste Manual',
    item_type,item_sub_type,equip_pos,weapon_class,
    18,                           -- ItemEffectType::StudyMagic
    78,                           -- magic_id for Haste
    0,0,0,0,0,0,
    durability,special_effect,
    sell_price,weight,swing_speed,level_requirement,gender_requirement,
    special_effect_value1,special_effect_value2,related_skill,hide_armor,is_skirt,
    stackable,is_dyeable,set_id,item_color,display_id,armor_class,attribute_pool_id
FROM items
WHERE item_id = 852
  AND NOT EXISTS (SELECT 1 FROM items WHERE item_id = 2000)
  AND NOT EXISTS (SELECT 1 FROM items WHERE name = 'Haste Manual');

COMMIT;

-- Note: To apply this migration, run this SQL against Binaries/Server/gamedata.db (e.g., using sqlite3).
-- The script intentionally uses a fixed item_id (2000). If your environment already has item_id 2000, edit the script and choose an unused id
-- or replace the first SELECT column with (SELECT COALESCE(MAX(item_id),0)+1 FROM items) to auto-assign a new id.
