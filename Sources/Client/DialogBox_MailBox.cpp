#include "DialogBox_MailBox.h"
#include "Game.h"
#include "Packet/PacketMailBox.h"
#include "IInput.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "TextInputManager.h"
#include "ItemSpriteMetadata.h"
#include "InventoryManager.h"
#include "AudioManager.h"
using namespace hb::client::sprite_id;
using namespace hb::shared::net;

DialogBox_MailBox::DialogBox_MailBox(CGame* game) : IDialogBox(DialogBoxId::MailBox, game)
{
    // The collision box for the dialog
    set_default_rect(237, 57, 252, 250); 
}

void DialogBox_MailBox::on_draw()
{
    short sX = m_x;
    short sY = m_y;
    short size_x = m_size_x;

    // Background (Using frame 2 from InterfaceNdGame2 for a solid background like CityHallMenu)
    draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);

    switch (m_mode) {
    case mode::list:
        DrawMode_List(sX, sY, size_x);
        break;
    case mode::read:
        DrawMode_Read(sX, sY, size_x);
        break;
    case mode::compose:
        DrawMode_Compose(sX, sY, size_x);
        break;
    }
}

void DialogBox_MailBox::DrawMode_List(short sX, short sY, short size_x)
{
    put_aligned_string(sX, sX + size_x, sY + 15, "--- MailBox ---", GameColors::UILabel);
    
    int y = 45;
    if (m_mails.empty()) {
        put_string(sX + 40, sY + y, "No messages.", GameColors::UITopMsgYellow);
    } else {
        for (const auto& mail : m_mails) {
            std::string prefix = mail.is_read ? "[Read] " : "[New] ";
            std::string text = prefix + mail.subject + " (" + mail.sender + ")";
            if (mouse_in({20, y, 180, 15})) {
                put_string(sX + 20, sY + y, text.c_str(), GameColors::UIWhite);
            } else {
                put_string(sX + 20, sY + y, text.c_str(), mail.is_read ? GameColors::UIDisabledMed : GameColors::UIMagicBlue);
            }
            
            y += 15;
            if (y > 200) break; 
        }
    }

    // Compose Button
    if (mouse_in({20, 220, 60, 15})) {
        put_string(sX + 20, sY + 220, "Compose", GameColors::UIWhite);
    } else {
        put_string(sX + 20, sY + 220, "Compose", GameColors::UITopMsgYellow);
    }

    // Close Button
    if (mouse_in({180, 220, 50, 15})) {
        put_string(sX + 180, sY + 220, "Close", GameColors::UIWhite);
    } else {
        put_string(sX + 180, sY + 220, "Close", GameColors::UITopMsgYellow);
    }
}

void DialogBox_MailBox::DrawMode_Read(short sX, short sY, short size_x)
{
    put_aligned_string(sX, sX + size_x, sY + 15, m_read_subject.c_str(), GameColors::UILabel);
    
    std::string senderStr = "From: " + m_read_sender;
    put_string(sX + 20, sY + 35, senderStr.c_str(), GameColors::UIWhite);
    
    put_string(sX + 20, sY + 55, m_read_body.c_str(), GameColors::UIDisabledMed);

    if (m_read_gold > 0 || m_read_item_id > 0) {
        if (mouse_in({20, 170, 105, 15})) {
            put_string(sX + 20, sY + 170, "Take Attachment", GameColors::UIWhite);
        } else {
            put_string(sX + 20, sY + 170, "Take Attachment", GameColors::UITopMsgYellow);
        }
    }

    if (mouse_in({180, 220, 50, 15})) {
        put_string(sX + 180, sY + 220, "Back", GameColors::UIWhite);
    } else {
        put_string(sX + 180, sY + 220, "Back", GameColors::UITopMsgYellow);
    }
}

void DialogBox_MailBox::DrawMode_Compose(short sX, short sY, short size_x)
{
    put_aligned_string(sX, sX + size_x, sY + 15, "--- Compose Mail ---", GameColors::UILabel);
    
    put_string(sX + 20, sY + 40, "To:", GameColors::UIWhite);
    if (m_compose_active_field != 1) {
        put_string(sX + 70, sY + 40, m_compose_receiver.empty() ? "Click to type" : m_compose_receiver.c_str(), GameColors::UIDisabledMed);
    }
    
    put_string(sX + 20, sY + 60, "Subject:", GameColors::UIWhite);
    if (m_compose_active_field != 2) {
        put_string(sX + 70, sY + 60, m_compose_subject.empty() ? "Click to type" : m_compose_subject.c_str(), GameColors::UIDisabledMed);
    }
    
    put_string(sX + 20, sY + 80, "Body:", GameColors::UIWhite);
    if (m_compose_active_field != 3) {
        put_string(sX + 70, sY + 80, m_compose_body.empty() ? "Click to type" : m_compose_body.c_str(), GameColors::UIDisabledMed);
    }
    
    if (m_compose_inventory_slot != -1 && m_game->m_player->m_item_list[m_compose_inventory_slot]) {
        CItem* item = m_game->m_player->m_item_list[m_compose_inventory_slot].get();
        CItem* item_cfg = m_game->get_item_config(item->m_id_num);
        auto item_draw = m_game->get_item_draw(item_cfg ? item_cfg->m_display_id : 0, item_atlas::pack, item_cfg ? item_cfg->sprite_is_female() : false);
        if (item->m_instance.item_color == 0) {
            item_draw.sprite->draw(sX + 70, sY + 115, item_draw.frame);
        } else {
            const auto& tint = m_game->m_color_palette[item->m_instance.item_color];
            item_draw.sprite->draw(sX + 70, sY + 115, item_draw.frame, hb::shared::sprite::DrawParams::tint(tint.r, tint.g, tint.b));
        }
        put_string(sX + 110, sY + 115, item->m_name, GameColors::UIOrange);
    } else {
        put_string(sX + 70, sY + 110, "Drag item to attach", GameColors::UIDisabledMed);
    }

    if (mouse_in({20, 220, 50, 15})) {
        put_string(sX + 20, sY + 220, "Send", GameColors::UIWhite);
    } else {
        put_string(sX + 20, sY + 220, "Send", GameColors::UITopMsgYellow);
    }

    if (mouse_in({180, 220, 50, 15})) {
        put_string(sX + 180, sY + 220, "Cancel", GameColors::UIWhite);
    } else {
        put_string(sX + 180, sY + 220, "Cancel", GameColors::UITopMsgYellow);
    }
}

bool DialogBox_MailBox::on_click()
{
    short sX = m_x;
    short sY = m_y;

    switch (m_mode) {
    case mode::list: return OnClick_List(sX, sY);
    case mode::read: return OnClick_Read(sX, sY);
    case mode::compose: return OnClick_Compose(sX, sY);
    }
    return false;
}

bool DialogBox_MailBox::OnClick_List(short sX, short sY)
{
    int y = 45;
    for (const auto& mail : m_mails) {
        if (mouse_in({20, y, 180, 15})) {
            hb::net::PacketRequestReadMail pkt{};
            pkt.header.msg_id = hb::shared::net::MsgId::RequestReadMail;
            pkt.mail_id = mail.mail_id;
            send_game_packet(pkt);
            return true;
        }
        y += 15;
        if (y > 200) break;
    }

    if (mouse_in({20, 220, 60, 15})) {
        m_mode = mode::compose;
        m_compose_receiver.clear();
        m_compose_subject.clear();
        m_compose_body.clear();
        m_compose_active_field = 0;
        return true;
    }

    if (mouse_in({180, 220, 50, 15})) {
        disable_this_dialog();
        return true;
    }

    return false;
}

bool DialogBox_MailBox::OnClick_Read(short sX, short sY)
{
    if (m_read_gold > 0 || m_read_item_id > 0) {
        if (mouse_in({20, 170, 105, 15})) {
            hb::net::PacketRequestTakeAttachment pkt{};
            pkt.header.msg_id = hb::shared::net::MsgId::RequestTakeAttachment;
            pkt.mail_id = m_read_mail_id;
            send_game_packet(pkt);
            
            m_read_gold = 0;
            m_read_item_id = 0;
            return true;
        }
    }

    if (mouse_in({180, 220, 50, 15})) {
        m_mode = mode::list;
        return true;
    }
    
    return false;
}

bool DialogBox_MailBox::OnClick_Compose(short sX, short sY)
{
    // Receiver field
    if (mouse_in({70, 40, 150, 15})) {
        text_input_manager::get().end_input();
        text_input_manager::get().start_input(sX + 70, sY + 40, 20, m_compose_receiver);
        text_input_manager::get().set_text_color(GameColors::UIOrange);
        m_compose_active_field = 1;
        return true;
    }
    
    // Subject field
    if (mouse_in({70, 60, 150, 15})) {
        text_input_manager::get().end_input();
        text_input_manager::get().start_input(sX + 70, sY + 60, 39, m_compose_subject);
        text_input_manager::get().set_text_color(GameColors::UIOrange);
        m_compose_active_field = 2;
        return true;
    }
    
    // Body field
    if (mouse_in({70, 80, 150, 15})) {
        m_compose_active_field = 3;
        text_input_manager::get().start_input(sX + 30, sY + 80, 200, m_compose_body);
        text_input_manager::get().set_text_color(GameColors::UIOrange);
        return true;
    }

    // Send button
    if (mouse_in({20, 220, 50, 15})) {
        text_input_manager::get().end_input();
        hb::net::PacketRequestSendMail pkt{};
        pkt.header.msg_id = hb::shared::net::MsgId::RequestSendMail;
        std::snprintf(pkt.receiver_name, sizeof(pkt.receiver_name), "%s", m_compose_receiver.c_str());
        std::snprintf(pkt.subject, sizeof(pkt.subject), "%s", m_compose_subject.c_str());
        std::snprintf(pkt.body, sizeof(pkt.body), "%s", m_compose_body.c_str());
        pkt.attached_gold = 0; // Gold attachment not implemented yet
        pkt.inventory_slot = m_compose_inventory_slot;
        
        send_game_packet(pkt);
        
        m_compose_receiver.clear();
        m_compose_subject.clear();
        m_compose_body.clear();
        m_compose_active_field = 0;
        m_compose_inventory_slot = -1;
        m_mode = mode::list;
        return true;
    }

    // Cancel button
    if (mouse_in({180, 220, 50, 15})) {
        text_input_manager::get().end_input();
        m_compose_active_field = 0;
        m_mode = mode::list;
        return true;
    }

    // Deselect input if clicked outside
    if (m_compose_active_field != 0) {
        text_input_manager::get().end_input();
        m_compose_active_field = 0;
    }

    return false;
}

bool DialogBox_MailBox::on_item_drop()
{
    if (m_mode != mode::compose) return false;

    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());
    
    if (mouse_x < m_x || mouse_x > m_x + m_size_x || mouse_y < m_y || mouse_y > m_y + m_size_y) {
        return false;
    }

    int item_id = CursorTarget::get_selected_id();
    if (item_id < 0 || item_id >= hb::shared::limits::MaxItems) return false;
    if (inventory_manager::get().warn_if_locked(item_id)) return false;

    m_compose_inventory_slot = item_id;
    audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
    return true;
}

bool DialogBox_MailBox::on_disable()
{
    if (m_compose_active_field != 0) {
        text_input_manager::get().end_input();
        m_compose_active_field = 0;
    }
    return true;
}
