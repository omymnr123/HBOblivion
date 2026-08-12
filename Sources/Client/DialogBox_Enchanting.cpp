#include "DialogBox_Enchanting.h"
#include "Game.h"
#include "IInput.h"
#include "Misc.h"
#include "AudioManager.h"
#include "PacketSendHelpers.h"
#include "Screen_OnGame.h"
#include "CursorTarget.h"
#include "Item/Item.h"
#include "InventoryManager.h"
#include "ItemNameFormatter.h"
#include "ItemSpriteMetadata.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>
#include <cstring>
#include "ItemTooltip.h"

using namespace hb::client::sprite_id;
using namespace hb::shared::item;
using namespace hb::shared::text;

// ────────────────────────────────────────────────────────────────
// Stat name tables — order matches the DB prefix/secondary IDs
// ────────────────────────────────────────────────────────────────

// Shards: material_type == 1, stat_id maps to AttributePrefixType
static const char* shard_stat_names[] = {
    "",               // 0  None
    "Critical",       // 1
    "Poisoning",      // 2
    "Righteous",      // 3
    "",               // 4  Reserved
    "Agile",          // 5
    "Light",          // 6
    "Sharp",          // 7
    "Endurance",      // 8  Strong
    "Ancient",        // 9
    "Cast Prob.",      // 10 Special
    "Mana Conv.",     // 11
    "Crit Chance",    // 12
    "Experience",     // 13 — mapped custom
    "Gold",           // 14 — mapped custom
    "Crush Chance",   // 15 — mapped custom
};
static constexpr int SHARD_STAT_COUNT = 16;

// Fragments: material_type == 2, stat_id maps to SecondaryEffectType
static const char* fragment_stat_names[] = {
    "",               // 0  None
    "Poison Res.",    // 1
    "Hit Ratio",      // 2
    "Defense Ratio",  // 3
    "HP Rec.",        // 4
    "SP Rec.",        // 5
    "MP Rec.",        // 6
    "Magic Res.",     // 7
    "Phys. Abs.",     // 8
    "Magic Abs.",     // 9
    "Consec. Atk",    // 10
    "Experience",     // 11
    "Gold",           // 12
    "Crush Damage",   // 13 — mapped custom
};
static constexpr int FRAGMENT_STAT_COUNT = 14;

// Maximum enchanting level
static constexpr int MAX_ENCHANT_LEVEL = 17;

const char* GetPrefixNameUI(int prefix) {
    if (prefix >= 0 && prefix < SHARD_STAT_COUNT) return shard_stat_names[prefix];
    return "Unknown";
}
const char* GetSecondaryNameUI(int sec) {
    if (sec >= 0 && sec < FRAGMENT_STAT_COUNT) return fragment_stat_names[sec];
    if (sec == 40) return "Hit Ratio %";
    return "Unknown";
}

// ────────────────────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────────────────────

DialogBox_Enchanting::DialogBox_Enchanting(CGame* game)
    : IDialogBox(DialogBoxId::Enchanting, game)
{
    set_default_rect(20, 55, PANEL_W, PANEL_H);
    m_active_tab = 0;
    m_target_item = -1;
}

DialogBox_EnchantingBag::DialogBox_EnchantingBag(CGame* game)
    : IDialogBox(DialogBoxId::EnchantingBag, game)
{
    set_default_rect(20 + 260 + 5, 55, PANEL_W, PANEL_H); // Default right of Enchanting
}

// ────────────────────────────────────────────────────────────────
// Utility: look up amount in the enchant_bag
// ────────────────────────────────────────────────────────────────

int DialogBox_EnchantingBag::get_bag_amount(int material_type, int stat_id, int level) const
{
    for (const auto& entry : m_game->m_player->m_enchant_bag) {
        if (entry.material_type == material_type &&
            entry.stat_id == stat_id &&
            entry.level == level)
            return static_cast<int>(entry.amount);
    }
    return 0;
}

// ════════════════════════════════════════════════════════════════
//                         DRAWING
// ════════════════════════════════════════════════════════════════

void DialogBox_Enchanting::on_draw()
{
    if (!m_game->ensure_item_configs_loaded()) return;

    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    draw_left_panel(m_x, m_y, mouse_x, mouse_y);
}

// ────────────────────────────────────────────────────────────────
// LEFT PANEL — "Enchanting"
// ────────────────────────────────────────────────────────────────

void DialogBox_Enchanting::draw_left_panel(short sX, short sY, short mouse_x, short mouse_y)
{
    auto* renderer = m_game->m_Renderer;

    // ── Semi-transparent dark background ──
    renderer->draw_rect_filled(sX, sY, PANEL_W, PANEL_H,
        hb::shared::render::Color{0, 0, 0, 160});
    // ── Thin border ──
    renderer->draw_rect_outline(sX, sY, PANEL_W, PANEL_H,
        hb::shared::render::Color{120, 100, 70}, 1);

    // ── Title: "Enchanting" ──
    put_aligned_string(sX, sX + PANEL_W, sY + 8, "Enchanting",
        GameColors::UIYellow);

    // ── Horizontal rule under title ──
    renderer->draw_rect_filled(sX + 10, sY + 25, PANEL_W - 20, 1,
        hb::shared::render::Color{120, 100, 70});

    // ── Tabs: Enchant  Disenchant  Recover ──
    const char* tabs[3] = { "Enchant", "Disenchant", "Recover" };
    const int tab_x[3]  = { sX + 15, sX + 90, sX + 185 };
    for (int i = 0; i < 3; ++i) {
        bool hover = (mouse_x >= tab_x[i] && mouse_x <= tab_x[i] + 70 &&
                      mouse_y >= sY + 30 && mouse_y <= sY + 45);
        if (m_active_tab == i) {
            // Active tab: underlined, bright color
            put_string(tab_x[i], sY + 32, tabs[i], GameColors::UIYellow);
            renderer->draw_rect_filled(tab_x[i], sY + 44, 
                static_cast<int>(strlen(tabs[i])) * 7, 1,
                hb::shared::render::Color{200, 200, 25});
        } else if (hover) {
            put_string(tab_x[i], sY + 32, tabs[i], GameColors::UIWhite);
        } else {
            put_string(tab_x[i], sY + 32, tabs[i],
                hb::shared::render::Color{140, 140, 140});
        }
    }

    // ── Description text (changes per tab) ──
    int textY = sY + 55;
    switch (m_active_tab) {
    case 0: // Enchant
        put_string(sX + 15, textY,      "Drop an item here to enchant it.", GameColors::UIWhite);
        put_string(sX + 15, textY + 15, "Enchanting the first stat requires shards.", GameColors::UIWhite);
        put_string(sX + 15, textY + 30, "The second stat requires fragments.", GameColors::UIWhite);
        put_string(sX + 15, textY + 45, "Disenchant items to get both.", GameColors::UIWhite);
        put_string(sX + 15, textY + 60, "You can also combine shards/fragments.", GameColors::UIWhite);
        break;
    case 1: // Disenchant
        put_string(sX + 15, textY,      "Drop an item here to disenchant it.", GameColors::UIWhite);
        put_string(sX + 15, textY + 15, "The first stat disenchants to shards.", GameColors::UIWhite);
        put_string(sX + 15, textY + 30, "The second stat disenchants to fragments.", GameColors::UIWhite);
        put_string(sX + 15, textY + 45, "Disenchanting will destroy the item.", GameColors::UIRed);
        break;
    case 2: // Recover
        put_aligned_string(sX + 15, sX + PANEL_W - 15, textY, "Recover queue:", GameColors::UIWhite);
        {
            int ry = textY + 20;
            int hover_index = -1;
            for (size_t i = 0; i < m_game->m_player->m_recover_queue.size(); ++i) {
                const auto& entry = m_game->m_player->m_recover_queue[i];
                CItem* cfg = m_game->get_item_config(entry.item_id);
                if (!cfg) continue;
                
                bool is_hover = (mouse_x >= sX + 15 && mouse_x <= sX + PANEL_W - 15 && mouse_y >= ry && mouse_y <= ry + 14);
                if (is_hover) hover_index = static_cast<int>(i);
                
                CItem fakeItem;
                fakeItem.m_id_num = entry.item_id;
                fakeItem.m_instance.item_color = entry.item_color;
                fakeItem.m_instance.prefix_type = entry.prefix_type;
                fakeItem.m_instance.prefix_value = entry.prefix_value;
                fakeItem.m_instance.secondary_type = entry.secondary_type;
                fakeItem.m_instance.secondary_value = entry.secondary_value;
                fakeItem.m_instance.enchant_bonus = entry.enchant_bonus;
                fakeItem.m_instance.special_effect_value1 = entry.spec_value1;
                fakeItem.m_instance.special_effect_value2 = entry.spec_value2;
                fakeItem.m_instance.special_effect_value3 = entry.spec_value3;
                
                auto itemInfo = item_name_formatter::get().format(&fakeItem);
                
                put_aligned_string(sX + 15, sX + PANEL_W - 15, ry, itemInfo.name.c_str(), is_hover ? GameColors::UIYellow : GameColors::UIGreen);
                
                if (is_hover) {
                    item_tooltip tooltip;
                    hb::shared::render::Color nameColor = GameColors::UIWhite;
                    if (entry.item_color != 0) {
                        const auto& tint = m_game->m_color_palette[entry.item_color];
                        nameColor = hb::shared::render::Color{tint.r, tint.g, tint.b};
                    }
                    tooltip.add_line(itemInfo.name, nameColor);
                    
                    auto effect = itemInfo.effect_text();
                    if (!effect.empty()) tooltip.add_line(effect, GameColors::UIDescription);
                    
                    auto extra = itemInfo.extra_text();
                    if (!extra.empty()) tooltip.add_line(extra, GameColors::UIDescription);
                    
                    tooltip.add_line(std::format("Cost: {} Shards, {} Fragments", entry.shards_yield, entry.fragments_yield), hb::shared::render::Color{255, 100, 100});
                    
                    tooltip.draw(mouse_x, mouse_y + 15, renderer);
                }
                
                ry += 14;
                if (ry > sY + SLOT_Y - 5) break;
            }
            if (m_game->m_player->m_recover_queue.empty())
                put_aligned_string(sX + 15, sX + PANEL_W - 15, textY + 20, "No items in recovery queue.", GameColors::UIDescription);
        }
        break;
    }

    // ── Item slot (dark sunken rectangle) — only for Enchant/Disenchant ──
    if (m_active_tab == 0 || m_active_tab == 1) {
        // Dark slot background
        renderer->draw_rect_filled(sX + SLOT_X, sY + SLOT_Y, SLOT_W, SLOT_H,
            hb::shared::render::Color{0, 0, 0, 200});
        // Slot border
        renderer->draw_rect_outline(sX + SLOT_X, sY + SLOT_Y, SLOT_W, SLOT_H,
            hb::shared::render::Color{80, 70, 50}, 1);

        // Draw item if one is placed
        if (m_target_item != -1 && m_game->m_player->m_item_list[m_target_item]) {
            auto* item = m_game->m_player->m_item_list[m_target_item].get();
            char item_color = item->m_instance.item_color;
            CItem* cfg = m_game->get_item_config(item->m_id_num);
            if (cfg) {
                auto upg_draw = m_game->get_item_draw(cfg->m_display_id, item_atlas::pack, cfg->sprite_is_female());
                int drawX = sX + SLOT_X + SLOT_W / 2;
                int drawY = sY + SLOT_Y + SLOT_H / 2 + 10;
                if (item_color == 0)
                    upg_draw.sprite->draw(drawX, drawY, upg_draw.frame);
                else {
                    const auto& tint = m_game->m_color_palette[item_color];
                    upg_draw.sprite->draw(drawX, drawY, upg_draw.frame,
                        hb::shared::sprite::DrawParams::tint(tint.r, tint.g, tint.b));
                }

                // Item name and stats below slot
                auto itemInfo = item_name_formatter::get().format(item);
                int infoY = sY + SLOT_Y + SLOT_H + 5;
                put_aligned_string(sX + 5, sX + PANEL_W - 5, infoY, itemInfo.name.c_str(), GameColors::UIWhite);
                auto effect = itemInfo.effect_text();
                auto extra  = itemInfo.extra_text();
                if (!effect.empty())
                    put_aligned_string(sX + 5, sX + PANEL_W - 5, infoY + 14, effect.c_str(), GameColors::UIDescription);
                if (!extra.empty())
                    put_aligned_string(sX + 5, sX + PANEL_W - 5, infoY + 28, extra.c_str(), GameColors::UIDescription);
            }
        }

        // ── Action Button (Enchant / Disenchant) ──
        if (m_target_item != -1) {
            auto* item = m_game->m_player->m_item_list[m_target_item].get();
            if (m_active_tab == 0) {
                // Enchant Tab: up to 2 buttons
                int btn_y = sY + ACTION_BTN_Y;
                if (item->m_instance.prefix_type > 0 && item->m_instance.prefix_value < 15) {
                    bool hover = (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                                  mouse_y >= btn_y && mouse_y <= btn_y + ACTION_BTN_H);
                    renderer->draw_rect_filled(sX + ACTION_BTN_X, btn_y, ACTION_BTN_W, ACTION_BTN_H,
                        hb::shared::render::Color{30, 30, 30, 200});
                    renderer->draw_rect_outline(sX + ACTION_BTN_X, btn_y, ACTION_BTN_W, ACTION_BTN_H,
                        hb::shared::render::Color{120, 100, 70}, 1);
                    std::string action_text = std::format("Enchant {} to +{}", GetPrefixNameUI(item->m_instance.prefix_type), item->m_instance.prefix_value + 1);
                    put_aligned_string(sX + ACTION_BTN_X, sX + ACTION_BTN_X + ACTION_BTN_W, btn_y + 3,
                        action_text.c_str(), hover ? GameColors::UIWhite : GameColors::UIDescription);
                    btn_y += ACTION_BTN_H + 5;
                }
                if (item->m_instance.secondary_type > 0 && item->m_instance.secondary_value < 15) {
                    bool hover = (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                                  mouse_y >= btn_y && mouse_y <= btn_y + ACTION_BTN_H);
                    renderer->draw_rect_filled(sX + ACTION_BTN_X, btn_y, ACTION_BTN_W, ACTION_BTN_H,
                        hb::shared::render::Color{30, 30, 30, 200});
                    renderer->draw_rect_outline(sX + ACTION_BTN_X, btn_y, ACTION_BTN_W, ACTION_BTN_H,
                        hb::shared::render::Color{120, 100, 70}, 1);
                    std::string action_text = std::format("Enchant {} to +{}", GetSecondaryNameUI(item->m_instance.secondary_type), item->m_instance.secondary_value + 1);
                    put_aligned_string(sX + ACTION_BTN_X, sX + ACTION_BTN_X + ACTION_BTN_W, btn_y + 3,
                        action_text.c_str(), hover ? GameColors::UIWhite : GameColors::UIDescription);
                }
            } else {
                // Disenchant Tab: 1 button
                bool hover_action = (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                                     mouse_y >= sY + ACTION_BTN_Y && mouse_y <= sY + ACTION_BTN_Y + ACTION_BTN_H);
                renderer->draw_rect_filled(sX + ACTION_BTN_X, sY + ACTION_BTN_Y, ACTION_BTN_W, ACTION_BTN_H,
                    hb::shared::render::Color{30, 30, 30, 200});
                renderer->draw_rect_outline(sX + ACTION_BTN_X, sY + ACTION_BTN_Y, ACTION_BTN_W, ACTION_BTN_H,
                    hb::shared::render::Color{120, 100, 70}, 1);
                put_aligned_string(sX + ACTION_BTN_X, sX + ACTION_BTN_X + ACTION_BTN_W, sY + ACTION_BTN_Y + 3,
                    "Disenchant", hover_action ? GameColors::UIWhite : GameColors::UIDescription);
            }
        }
    }

    // ── "Enchanting Bag" button ──
    {
        bool hover = (mouse_x >= sX + BAG_BTN_X && mouse_x <= sX + BAG_BTN_X + BAG_BTN_W &&
                      mouse_y >= sY + BAG_BTN_Y && mouse_y <= sY + BAG_BTN_Y + BAG_BTN_H);
        renderer->draw_rect_filled(sX + BAG_BTN_X, sY + BAG_BTN_Y, BAG_BTN_W, BAG_BTN_H,
            hb::shared::render::Color{30, 30, 30, 200});
        renderer->draw_rect_outline(sX + BAG_BTN_X, sY + BAG_BTN_Y, BAG_BTN_W, BAG_BTN_H,
            hb::shared::render::Color{120, 100, 70}, 1);
        put_aligned_string(sX + BAG_BTN_X, sX + BAG_BTN_X + BAG_BTN_W, sY + BAG_BTN_Y + 3,
            "Enchanting Bag", hover ? GameColors::UIWhite : GameColors::UIDescription);
    }
}

// ────────────────────────────────────────────────────────────────
// RIGHT PANEL — "Enchanting Bag" (now in its own class)
// ────────────────────────────────────────────────────────────────

void DialogBox_EnchantingBag::on_draw()
{
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    auto* renderer = m_game->m_Renderer;
    short rX = m_x;
    short sY = m_y;

    // ── Semi-transparent dark background ──
    renderer->draw_rect_filled(rX, sY, PANEL_W, PANEL_H, hb::shared::render::Color{0, 0, 0, 160});
    renderer->draw_rect_outline(rX, sY, PANEL_W, PANEL_H, hb::shared::render::Color{120, 100, 70}, 1);

    // ── Title ──
    put_aligned_string(rX, rX + PANEL_W, sY + 8, "Enchanting Bag", GameColors::UIWhite);
    
    renderer->draw_rect_filled(rX + 10, sY + 25, PANEL_W - 20, 1, hb::shared::render::Color{120, 100, 70});

    // ── Tabs ──
    hb::shared::render::Color shardColor = (m_active_tab == 0) ? hb::shared::render::Color{0, 255, 0} : hb::shared::render::Color{19, 104, 169};
    hb::shared::render::Color fragColor = (m_active_tab == 1) ? hb::shared::render::Color{0, 255, 0} : hb::shared::render::Color{19, 104, 169};

    put_string(rX + 130, sY + 35, "Shards", shardColor);
    put_string(rX + 330, sY + 35, "Fragments", fragColor);

    // ── Level Headers ──
    put_string(rX + 100, sY + 65, "Lv.", hb::shared::render::Color{255, 168, 0});
    int addx = 25;
    for (int i = 1; i < 18; i++) {
        char cTxt[4];
        std::snprintf(cTxt, sizeof(cTxt), "%d", i);
        put_string(rX + 100 + addx, sY + 65, cTxt, hb::shared::render::Color{255, 168, 0});
        addx += 21;
    }
    
    renderer->draw_rect_filled(rX + 10, sY + 80, PANEL_W - 20, 1, hb::shared::render::Color{60, 50, 40});

    // ── Rows ──
    struct StatDef {
        const char* name;
        int id;
    };
    
    StatDef shards_stats[] = {
        {"Poisoning", 2},
        {"Light", 6},
        {"Endurance", 8},
        {"Cast Prob.", 10},
        {"Mana Conv.", 11}
    };
    
    StatDef fragments_stats[] = {
        {"Poison Res.", 1},
        {"Hit Ratio", 2},
        {"Defense Ratio", 3},
        {"HP Rec.", 4},
        {"SP Rec.", 5},
        {"MP Rec.", 6},
        {"Magic Res.", 7},
        {"Phys. Abs.", 8},
        {"Magic Abs.", 9},
        {"Experience", 11},
        {"Gold", 12}
    };

    StatDef* current_stats = (m_active_tab == 0) ? shards_stats : fragments_stats;
    int num_stats = (m_active_tab == 0) ? 5 : 11;
    
    int addy = 20;
    for (int x = 0; x < num_stats; x++) {
        put_string(rX + 20, sY + 70 + addy, current_stats[x].name, GameColors::UIWhite);
        
        int addx = 25;
        for (int lv = 1; lv <= 17; lv++) {
            int count = get_bag_amount(m_active_tab, current_stats[x].id, lv);
            char cMsg[16];
            std::snprintf(cMsg, sizeof(cMsg), "%d", count);
            
            hb::shared::render::Color textColor = (count > 0) ? hb::shared::render::Color{220, 60, 60} : hb::shared::render::Color{100, 100, 100};
            if (m_active_tab == 1 && count > 0) textColor = hb::shared::render::Color{60, 200, 60};
            
            if (count > 0 && mouse_x >= rX + 100 + addx && mouse_x <= rX + 100 + addx + 19 && mouse_y >= sY + 70 + addy && mouse_y <= sY + 85 + addy) {
                textColor = hb::shared::render::Color{255, 255, 0};
            }
            
            put_string(rX + 100 + addx, sY + 70 + addy, cMsg, textColor);
            addx += 21;
        }
        addy += 15;
    }
    
    // Draw popup if active
    if (m_show_popup) {
        int pw = 120;
        int ph = 90;
        int px = m_popup_x;
        int py = m_popup_y;
        
        // Ensure it doesn't go off screen
        if (px + pw > 800) px = 800 - pw;
        if (py + ph > 600) py = 600 - ph;

        renderer->draw_rect_filled(px, py, pw, ph, hb::shared::render::Color{0, 0, 0, 230});
        renderer->draw_rect_outline(px, py, pw, ph, hb::shared::render::Color{100, 100, 100}, 1);

        // Title
        put_aligned_string(px, px + pw, py + 8, m_popup_title.c_str(), GameColors::UIRed);
        
        // Options
        const char* opts[4] = {"Withdraw", "Upgrade all", "Upgrade one", "Cancel"};
        for(int i=0; i<4; i++) {
            int opt_y = py + 30 + i*14;
            bool hover = (mouse_x >= px + 10 && mouse_x <= px + pw - 10 && mouse_y >= opt_y && mouse_y <= opt_y + 14);
            put_aligned_string(px, px + pw, opt_y, opts[i], hover ? GameColors::UIYellow : hb::shared::render::Color{60, 110, 220});
        }
    }
    
    // ── Bottom text ──
    int bottomY = sY + PANEL_H - 40;
    put_aligned_string(rX, rX + PANEL_W, bottomY,
        "Drop enchanting ingredients from your inventory to this bag to deposit them",
        GameColors::UIDescription);

    // ── "Deposit All" link ──
    {
        int linkY = bottomY + 16;
        bool hover = (mouse_x >= rX + PANEL_W / 2 - 30 && mouse_x <= rX + PANEL_W / 2 + 30 && mouse_y >= linkY && mouse_y <= linkY + 14);
        put_aligned_string(rX, rX + PANEL_W, linkY, "Deposit All", hover ? GameColors::UIWhite : GameColors::UITopMsgYellow);
    }
}

// ════════════════════════════════════════════════════════════════
//                         CLICK HANDLING
// ════════════════════════════════════════════════════════════════

bool DialogBox_Enchanting::on_click()
{
    short sX = m_x;
    short sY = m_y;
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    // ── Tab clicks ──
    const int tab_x[3] = { sX + 15, sX + 90, sX + 185 };
    for (int i = 0; i < 3; ++i) {
        if (mouse_x >= tab_x[i] && mouse_x <= tab_x[i] + 70 &&
            mouse_y >= sY + 30 && mouse_y <= sY + 45) {
            if (m_active_tab != i) {
                // Unlock item when switching away from enchant/disenchant
                if (m_target_item != -1 && (i == 2)) {
                    inventory_manager::get().unlock_item(m_target_item);
                    m_target_item = -1;
                }
                m_active_tab = i;
            }
            audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
            return true;
        }
    }

    // ── "Enchanting Bag" button click ──
    if (mouse_x >= sX + BAG_BTN_X && mouse_x <= sX + BAG_BTN_X + BAG_BTN_W &&
        mouse_y >= sY + BAG_BTN_Y && mouse_y <= sY + BAG_BTN_Y + BAG_BTN_H) {
        // Toggle the bag window
        m_game->get_dialog_box_manager().toggle_dialog_box(DialogBoxId::EnchantingBag);
        audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
        return true;
    }

    // ── Action Button and Slot Click ──
    if ((m_active_tab == 0 || m_active_tab == 1) && m_target_item != -1) {
        // If click is on the action button, trigger action
        if (m_active_tab == 0) {
            auto* item = m_game->m_player->m_item_list[m_target_item].get();
            int btn_y = sY + ACTION_BTN_Y;
            if (item->m_instance.prefix_type > 0 && item->m_instance.prefix_value < 15) {
                if (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                    mouse_y >= btn_y && mouse_y <= btn_y + ACTION_BTN_H) {
                    hb::net::PacketRequestEnchantAction req{};
                    req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                    req.action_type = hb::shared::net::CommonType::EnchantItem;
                    req.inventory_slot = m_target_item;
                    req.target_stat_id = 0; // Prefix
                    send_game_packet(req);
                    inventory_manager::get().unlock_item(m_target_item);
                    m_target_item = -1;
                    audio_manager::get().play_game_sound(sound_type::effect, 29, 0);
                    return true;
                }
                btn_y += ACTION_BTN_H + 5;
            }
            if (item->m_instance.secondary_type > 0 && item->m_instance.secondary_value < 15) {
                if (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                    mouse_y >= btn_y && mouse_y <= btn_y + ACTION_BTN_H) {
                    hb::net::PacketRequestEnchantAction req{};
                    req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                    req.action_type = hb::shared::net::CommonType::EnchantItem;
                    req.inventory_slot = m_target_item;
                    req.target_stat_id = 1; // Secondary
                    send_game_packet(req);
                    inventory_manager::get().unlock_item(m_target_item);
                    m_target_item = -1;
                    audio_manager::get().play_game_sound(sound_type::effect, 29, 0);
                    return true;
                }
            }
        } else if (m_active_tab == 1) {
            if (mouse_x >= sX + ACTION_BTN_X && mouse_x <= sX + ACTION_BTN_X + ACTION_BTN_W &&
                mouse_y >= sY + ACTION_BTN_Y && mouse_y <= sY + ACTION_BTN_Y + ACTION_BTN_H) {
                hb::net::PacketRequestEnchantAction req{};
                req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                req.action_type = hb::shared::net::CommonType::DisenchantItem;
                req.inventory_slot = m_target_item;
                req.target_stat_id = 0;
                send_game_packet(req);
                inventory_manager::get().unlock_item(m_target_item);
                m_target_item = -1;
                audio_manager::get().play_game_sound(sound_type::effect, 29, 0);
                return true;
            }
        }

        // If click is on the slot area, remove the item
        if (mouse_x >= sX + SLOT_X && mouse_x <= sX + SLOT_X + SLOT_W &&
            mouse_y >= sY + SLOT_Y && mouse_y <= sY + SLOT_Y + SLOT_H) {
            inventory_manager::get().unlock_item(m_target_item);
            m_target_item = -1;
            audio_manager::get().play_game_sound(sound_type::effect, 29, 0);
            return true;
        }
    }

    // ── Recover action ──
    if (m_active_tab == 2) {
        int textY = sY + 55;
        int ry = textY + 20;
        for (size_t i = 0; i < m_game->m_player->m_recover_queue.size(); ++i) {
            CItem* cfg = m_game->get_item_config(m_game->m_player->m_recover_queue[i].item_id);
            if (!cfg) continue;
            if (mouse_x >= sX + 15 && mouse_x <= sX + PANEL_W - 15 && mouse_y >= ry && mouse_y <= ry + 14) {
                hb::net::PacketRequestEnchantAction req{};
                req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                req.action_type = hb::shared::net::CommonType::ReqRecoverItem;
                req.inventory_slot = m_game->m_player->m_recover_queue[i].db_id; // Pass the DB ID of the queue entry
                req.target_stat_id = 0;
                send_game_packet(req);
                audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
                return true;
            }
            ry += 14;
            if (ry > sY + SLOT_Y - 5) break;
        }
    }

    return false;
}

bool DialogBox_EnchantingBag::on_click()
{
    short rX = m_x;
    short sY = m_y;
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    // Tabs
    if (mouse_y >= sY + 30 && mouse_y <= sY + 50) {
        if (mouse_x >= rX + 120 && mouse_x <= rX + 190) {
            m_active_tab = 0;
            audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
            return true;
        }
        if (mouse_x >= rX + 320 && mouse_x <= rX + 390) {
            m_active_tab = 1;
            audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
            return true;
        }
    }

    struct StatDef { const char* name; int id; };
    StatDef shards_stats[] = { {"Poisoning", 2}, {"Light", 6}, {"Endurance", 8}, {"Cast Prob.", 10}, {"Mana Conv.", 11} };
    StatDef fragments_stats[] = { {"Poison Res.", 1}, {"Hit Ratio", 2}, {"Defense Ratio", 3}, {"HP Rec.", 4}, {"SP Rec.", 5}, {"MP Rec.", 6}, {"Magic Res.", 7}, {"Phys. Abs.", 8}, {"Magic Abs.", 9}, {"Experience", 11}, {"Gold", 12} };
    StatDef* current_stats = (m_active_tab == 0) ? shards_stats : fragments_stats;
    int num_stats = (m_active_tab == 0) ? 5 : 11;

    // Handle popup click first
    if (m_show_popup) {
        int pw = 120;
        int ph = 90;
        int px = m_popup_x;
        int py = m_popup_y;
        if (px + pw > 800) px = 800 - pw;
        if (py + ph > 600) py = 600 - ph;

        // Check if click inside popup
        if (mouse_x >= px && mouse_x <= px + pw && mouse_y >= py && mouse_y <= py + ph) {
            for(int i=0; i<4; i++) {
                int opt_y = py + 30 + i*14;
                if (mouse_x >= px + 10 && mouse_x <= px + pw - 10 && mouse_y >= opt_y && mouse_y <= opt_y + 14) {
                    if (i == 0) {
                        // Withdraw
                        hb::net::PacketRequestEnchantAction req{};
                        req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                        req.action_type = (m_popup_tab == 0) ? hb::shared::net::CommonType::WithdrawShard : hb::shared::net::CommonType::WithdrawFragment;
                        req.inventory_slot = m_popup_level;
                        req.target_stat_id = m_popup_stat_id;
                        send_game_packet(req);
                    } else if (i == 1) {
                        // Upgrade all
                        hb::net::PacketRequestEnchantAction req{};
                        req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                        req.action_type = (m_popup_tab == 0) ? hb::shared::net::CommonType::UpgradeShardAll : hb::shared::net::CommonType::UpgradeFragmentAll;
                        req.inventory_slot = m_popup_level;
                        req.target_stat_id = m_popup_stat_id;
                        send_game_packet(req);
                    } else if (i == 2) {
                        // Upgrade one
                        hb::net::PacketRequestEnchantAction req{};
                        req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
                        req.action_type = (m_popup_tab == 0) ? hb::shared::net::CommonType::UpgradeShard : hb::shared::net::CommonType::UpgradeFragment;
                        req.inventory_slot = m_popup_level;
                        req.target_stat_id = m_popup_stat_id;
                        send_game_packet(req);
                    } else if (i == 3) {
                        // Cancel
                    }
                    m_show_popup = false;
                    audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
                    return true;
                }
            }
        }
        // Clicked outside popup, close it
        m_show_popup = false;
        return true;
    }

    int addy = 20;
    for (int x = 0; x < num_stats; x++) {
        int addx = 25;
        for (int lv = 1; lv <= 17; lv++) {
            int count = get_bag_amount(m_active_tab, current_stats[x].id, lv);
            if (count > 0) {
                if (mouse_x >= rX + 100 + addx && mouse_x <= rX + 100 + addx + 19 && mouse_y >= sY + 70 + addy && mouse_y <= sY + 85 + addy) {
                    m_show_popup = true;
                    m_popup_stat_id = current_stats[x].id;
                    m_popup_level = lv;
                    m_popup_tab = m_active_tab;
                    m_popup_x = mouse_x;
                    m_popup_y = mouse_y;
                    
                    char tbuf[64];
                    std::snprintf(tbuf, sizeof(tbuf), "%s Lv.%d", current_stats[x].name, lv);
                    m_popup_title = tbuf;

                    audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
                    return true;
                }
            }
            addx += 21;
        }
        addy += 15;
    }

    // "Deposit All" link click
    int linkY = sY + PANEL_H - 24;
    if (mouse_x >= rX + PANEL_W / 2 - 30 && mouse_x <= rX + PANEL_W / 2 + 30 && mouse_y >= linkY && mouse_y <= linkY + 14) {
        hb::net::PacketRequestEnchantAction req{};
        req.header.msg_id = hb::shared::net::MsgId::RequestEnchantAction;
        req.action_type = hb::shared::net::CommonType::ReqDepositMaterials;
        req.inventory_slot = -1; // -1 indicates Deposit All
        req.target_stat_id = 0;
        send_game_packet(req);
        audio_manager::get().play_game_sound(sound_type::effect, 12, 5);
        return true;
    }
    
    return false;
}

// ════════════════════════════════════════════════════════════════
//                      PRESS / DRAG / DROP
// ════════════════════════════════════════════════════════════════

PressResult DialogBox_Enchanting::on_press()
{
    return PressResult::Normal;
}

bool DialogBox_Enchanting::on_item_drop()
{
    int item_id = CursorTarget::get_selected_id();
    if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
    if (inventory_manager::get().warn_if_locked(item_id)) return false;
    if (m_game->m_player->m_Controller.get_command() < 0) return false;
    CItem* cfg = m_game->get_item_config(m_game->m_player->m_item_list[item_id]->m_id_num);
    if (!cfg || cfg->get_equip_pos() == EquipPos::None) return false;

    if (m_active_tab == 0 || m_active_tab == 1) {
        if (m_target_item != -1) {
            inventory_manager::get().unlock_item(m_target_item);
        }
        m_target_item = item_id;
        inventory_manager::get().lock_item(item_id);
        audio_manager::get().play_game_sound(sound_type::effect, 29, 0);
        return true;
    }

    return false;
}

bool DialogBox_EnchantingBag::on_item_drop()
{
    int item_id = CursorTarget::get_selected_id();
    if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
    if (inventory_manager::get().warn_if_locked(item_id)) return false;
    if (m_game->m_player->m_Controller.get_command() < 0) return false;
    CItem* cfg = m_game->get_item_config(m_game->m_player->m_item_list[item_id]->m_id_num);
    if (!cfg || cfg->get_equip_pos() == EquipPos::None) return false;

    // Drop on right panel = deposit materials
    // (handled by server via packet)
    return false;
}

// ════════════════════════════════════════════════════════════════
//                     ENABLE / DISABLE
// ════════════════════════════════════════════════════════════════

bool DialogBox_Enchanting::on_enable(int type, int64_t v1, int v2, const char* string)
{
    if (is_enabled()) return true;
    m_target_item = -1;
    m_active_tab = 0;
    return true;
}

bool DialogBox_Enchanting::on_disable()
{
    if (m_target_item != -1) {
        inventory_manager::get().unlock_item(m_target_item);
    }
    m_target_item = -1;
    return true;
}
