#include "DialogBox_ExtraLoot.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GlobalDef.h"
#include "GameFonts.h"
#include "SpriteID.h"
#include "TextLibExt.h"
#include "IInput.h"
#include "PacketSendHelpers.h"
#include "NetMessages.h"

DialogBox_ExtraLoot::DialogBox_ExtraLoot(CGame* game)
    : IDialogBox(DialogBoxId::ExtraLoot, game)
{
    m_size_x = 250;
    m_size_y = 300;
    m_x = static_cast<short>((LOGICAL_WIDTH() - m_size_x) / 2);
    m_y = static_cast<short>((LOGICAL_HEIGHT() - m_size_y) / 2);
}

void DialogBox_ExtraLoot::clear_loot() {
    m_loot_list.clear();
}

void DialogBox_ExtraLoot::add_loot(uint32_t db_id, int item_id, const hb::shared::item::item_instance_data& data) {
    m_loot_list.push_back({ db_id, item_id, data });
}

bool DialogBox_ExtraLoot::on_enable(int type, int64_t v1, int v2, const char* string) {
    // Request the actual item list from the server
    auto pkt = hb::net::make_common_command(hb::shared::net::CommonType::ReqExtraLootList, m_game->m_player->m_player_x, m_game->m_player->m_player_y);
    send_game_packet(pkt);
    return true;
}

void DialogBox_ExtraLoot::on_update() {
    m_x = static_cast<short>((LOGICAL_WIDTH() - m_size_x) / 2);
    m_y = static_cast<short>((LOGICAL_HEIGHT() - m_size_y) / 2);
}

void DialogBox_ExtraLoot::on_draw() {
    if (!m_game->ensure_item_configs_loaded()) return;

    draw_new_dialog_box(hb::client::sprite_id::InterfaceNdGame2, m_x, m_y, 0);

    hb::shared::text::draw_text_aligned(GameFont::Bitmap1,
        m_x, m_y + 10, m_size_x, 15,
        "Extra Loot!",
        hb::shared::text::TextStyle::with_integrated_shadow(GameColors::UIWarningRed),
        hb::shared::text::Align::TopCenter);

    int offsetY = 35;
    int hover_index = -1;
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    for (size_t i = 0; i < m_loot_list.size(); ++i) {
        auto itemInfo = item_name_formatter::get().format(
            static_cast<short>(m_loot_list[i].item_id),
            m_loot_list[i].item_data
        );

        if (mouse_in({ 10, offsetY, m_size_x - 20, 15 })) {
            put_aligned_string(m_x, m_x + m_size_x, m_y + offsetY, itemInfo.name.c_str(), GameColors::UIYellow);
            hover_index = static_cast<int>(i);
        }
        else {
            if (itemInfo.is_special) {
                put_aligned_string(m_x, m_x + m_size_x, m_y + offsetY, itemInfo.name.c_str(), GameColors::UIItemName_Special);
            }
            else {
                put_aligned_string(m_x, m_x + m_size_x, m_y + offsetY, itemInfo.name.c_str(), GameColors::UIFormLabel);
            }
        }
        offsetY += 16;
    }

    if (hover_index != -1) {
        draw_tooltip(mouse_x, mouse_y, hover_index);
    }
}

void DialogBox_ExtraLoot::draw_tooltip(short mouse_x, short mouse_y, int index) {
    if (index < 0 || index >= static_cast<int>(m_loot_list.size())) return;

    auto itemInfo = item_name_formatter::get().format(
        static_cast<short>(m_loot_list[index].item_id),
        m_loot_list[index].item_data
    );

    int tX = mouse_x + 15;
    int tY = mouse_y + 15;

    if (itemInfo.is_special) {
        put_string(tX, tY, itemInfo.name.c_str(), GameColors::UIItemName_Special);
    }
    else {
        put_string(tX, tY, itemInfo.name.c_str(), GameColors::UIWhite);
    }

    int loc = 15;
    auto effect = itemInfo.effect_text();
    auto extra = itemInfo.extra_text();

    if (!effect.empty()) {
        put_string(tX, tY + loc, effect.c_str(), GameColors::UIWhite);
        loc += 15;
    }
    if (!extra.empty()) {
        put_string(tX, tY + loc, extra.c_str(), GameColors::UIWhite);
    }
}

bool DialogBox_ExtraLoot::on_click() {
    int offsetY = 35;
    for (size_t i = 0; i < m_loot_list.size(); ++i) {
        if (mouse_in({ 10, offsetY, m_size_x - 20, 15 })) {
            // Forma correcta que usa tu cliente para enviar comandos con datos (visto en DialogBox_ItemUpgrade.cpp)
            auto pkt = hb::net::make_common_command(hb::shared::net::CommonType::ReqClaimExtraLoot, m_game->m_player->m_player_x, m_game->m_player->m_player_y);
            pkt.v1 = static_cast<int32_t>(m_loot_list[i].db_id);
            send_game_packet(pkt);
            return true;
        }
        offsetY += 16;
    }
    return false;
}