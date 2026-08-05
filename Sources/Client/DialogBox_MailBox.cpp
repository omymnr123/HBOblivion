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
#include "ItemTooltip.h"
#include "ItemNameFormatter.h"
using namespace hb::client::sprite_id;
using namespace hb::shared::net;

DialogBox_MailBox::DialogBox_MailBox(CGame* game) : IDialogBox(DialogBoxId::MailBox, game)
{
    // The collision box for the dialog
    set_default_rect(237, 57, 252, 250); 
    for (int i = 0; i < 10; ++i) m_compose_inventory_slots[i] = -1;
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
                put_string(sX + 20, sY + y, text.c_str(), mail.is_read ? GameColors::UIBlack : GameColors::UIMagicBlue);
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
    
    hb::shared::text::draw_text_wrapped(GameFont::Default, sX + 20, sY + 55, size_x - 40, 90, m_read_body.c_str(), GameColors::UIOrange);

    bool has_attach = false;
    int y = 145;
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());

    int attach_count = 0;
    for (int i = 0; i < 10; ++i) {
        if (m_read_attachments[i].item_id > 0) {
            has_attach = true;
            if (attach_count >= m_scroll_offset && attach_count < m_scroll_offset + 5) {
                auto itemInfo = item_name_formatter::get().format(static_cast<short>(m_read_attachments[i].item_id));
                
                std::string itemName = itemInfo.name;
                if (m_read_attachments[i].item_count > 1) {
                    itemName += " (" + std::to_string(m_read_attachments[i].item_count) + ")";
                }

                int row = attach_count - m_scroll_offset;
                int x = 20;
                int y_item = 145 + row * 15;

                if (mouse_in({static_cast<short>(x), static_cast<short>(y_item), 150, 15})) {
                    put_string(sX + x, sY + y_item, itemName.c_str(), GameColors::UIWhite);
                    item_tooltip tooltip;
                    tooltip.add_line(itemName, itemInfo.is_special ? GameColors::UIItemName_Special : GameColors::UIWhite);
                    for (const auto& effect : itemInfo.effects) {
                        tooltip.add_line(effect.label + effect.value, GameColors::InfoGrayLight);
                    }
                    tooltip.draw(mouse_x, mouse_y + 15, m_game->m_Renderer);
                } else {
                    put_string(sX + x, sY + y_item, itemName.c_str(), itemInfo.is_special ? GameColors::UIItemName_Special : GameColors::UITopMsgYellow);
                }
            }
            attach_count++;
        }
    }

    if (m_read_gold > 0 || has_attach) {
        if (mouse_in({20, 220, 105, 15})) {
            put_string(sX + 20, sY + 220, "Take Attachment", GameColors::UIWhite);
        } else {
            put_string(sX + 20, sY + 220, "Take Attachment", GameColors::UITopMsgYellow);
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
    
    put_string(sX + 20, sY + 30, "To:", GameColors::UIWhite);
    if (m_compose_active_field != 1) {
        put_string(sX + 60, sY + 30, m_compose_receiver.empty() ? "Click to type" : m_compose_receiver.c_str(), m_compose_receiver.empty() ? GameColors::InfoGrayLight : GameColors::UIOrange);
    }
    
    put_string(sX + 20, sY + 50, "Subject:", GameColors::UIWhite);
    if (m_compose_active_field != 2) {
        put_string(sX + 75, sY + 50, m_compose_subject.empty() ? "Click to type" : m_compose_subject.c_str(), m_compose_subject.empty() ? GameColors::InfoGrayLight : GameColors::UIOrange);
    }
    
    put_string(sX + 20, sY + 70, "Body:", GameColors::UIWhite);
    if (m_compose_body.empty() && m_compose_active_field != 3) {
        put_string(sX + 60, sY + 70, "Click to type", GameColors::InfoGrayLight);
    } else {
        hb::shared::text::draw_text_wrapped(GameFont::Default, sX + 60, sY + 70, size_x - 70, 70, m_compose_body.c_str(), GameColors::UIOrange);
    }
    
    int y = 145;
    short mouse_x = static_cast<short>(hb::shared::input::get_mouse_x());
    short mouse_y = static_cast<short>(hb::shared::input::get_mouse_y());
    bool has_attach = false;

    int attach_count = 0;
    for (int i = 0; i < 10; ++i) {
        int slot = m_compose_inventory_slots[i];
        if (slot != -1 && m_game->m_player->m_item_list[slot]) {
            has_attach = true;
            if (attach_count >= m_scroll_offset && attach_count < m_scroll_offset + 5) {
                CItem* item = m_game->m_player->m_item_list[slot].get();
                auto itemInfo = item_name_formatter::get().format(item);
                
                std::string itemName = itemInfo.name;
                if (item->m_instance.count > 1) {
                    itemName += " (" + std::to_string(item->m_instance.count) + ")";
                }

                int row = attach_count - m_scroll_offset;
                int x = 20;
                int y_item = 145 + row * 15;

                if (mouse_in({static_cast<short>(x), static_cast<short>(y_item), 150, 15})) {
                    put_string(sX + x, sY + y_item, itemName.c_str(), GameColors::UIWhite);
                    item_tooltip tooltip;
                    tooltip.add_line(itemName, itemInfo.is_special ? GameColors::UIItemName_Special : GameColors::UIWhite);
                    for (const auto& effect : itemInfo.effects) {
                        tooltip.add_line(effect.label + effect.value, GameColors::InfoGrayLight);
                    }
                    tooltip.draw(mouse_x, mouse_y + 15, m_game->m_Renderer);
                } else {
                    put_string(sX + x, sY + y_item, itemName.c_str(), itemInfo.is_special ? GameColors::UIItemName_Special : GameColors::UITopMsgYellow);
                }
            }
            attach_count++;
        }
    }

    if (!has_attach) {
        put_string(sX + 20, sY + 145, "Drag items to attach (max 10)", GameColors::UIDisabledMed);
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

    // Handle scroll wheel
    int z = hb::shared::input::get_mouse_wheel_delta();
    if (z != 0 && (m_mode == mode::read || m_mode == mode::compose)) {
        if (mouse_in({20, 145, 180, 75})) {
            if (z > 0) m_scroll_offset--;
            if (z < 0) m_scroll_offset++;
            if (m_scroll_offset < 0) m_scroll_offset = 0;
            // The maximum offset is handled implicitly because we just won't show anything beyond attach_count
        }
    }

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
    bool has_attach = m_read_gold > 0;
    for (int i = 0; i < 10; ++i) {
        if (m_read_attachments[i].item_id > 0) has_attach = true;
    }

    if (has_attach) {
        if (mouse_in({20, 220, 105, 15})) {
            hb::net::PacketRequestTakeAttachment pkt{};
            pkt.header.msg_id = hb::shared::net::MsgId::RequestTakeAttachment;
            pkt.mail_id = m_read_mail_id;
            send_game_packet(pkt);
            
            m_read_gold = 0;
            for (int i = 0; i < 10; ++i) {
                m_read_attachments[i].item_id = 0;
                m_read_attachments[i].item_count = 0;
            }
            return true;
        }
    }

    if (mouse_in({180, 220, 50, 15})) {
        m_mode = mode::list;
        m_scroll_offset = 0;
        return true;
    }
    
    return false;
}

bool DialogBox_MailBox::OnClick_Compose(short sX, short sY)
{
    // Receiver field
    if (mouse_in({60, 30, 150, 15})) {
        text_input_manager::get().end_input();
        text_input_manager::get().start_input(sX + 60, sY + 30, 20, m_compose_receiver);
        text_input_manager::get().set_text_color(GameColors::UIOrange);
        m_compose_active_field = 1;
        return true;
    }
    
    // Subject field
    if (mouse_in({75, 50, 150, 15})) {
        text_input_manager::get().end_input();
        text_input_manager::get().start_input(sX + 75, sY + 50, 39, m_compose_subject);
        text_input_manager::get().set_text_color(GameColors::UIOrange);
        m_compose_active_field = 2;
        return true;
    }
    
    // Body field
    if (mouse_in({60, 70, 150, 70})) {
        m_compose_active_field = 3;
        text_input_manager::get().end_input();
        text_input_manager::get().start_input(-1000, -1000, 200, m_compose_body);
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
        
        for (int i = 0; i < 10; ++i) {
            pkt.inventory_slots[i] = m_compose_inventory_slots[i];
        }
        
        send_game_packet(pkt);
        
        m_compose_receiver.clear();
        m_compose_subject.clear();
        m_compose_body.clear();
        m_compose_active_field = 0;
        m_scroll_offset = 0;
        for (int i = 0; i < 10; ++i) m_compose_inventory_slots[i] = -1;
        m_mode = mode::list;
        return true;
    }

    // Cancel button
    if (mouse_in({180, 220, 50, 15})) {
        text_input_manager::get().end_input();
        m_compose_active_field = 0;
        m_scroll_offset = 0;
        for (int i = 0; i < 10; ++i) m_compose_inventory_slots[i] = -1;
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

    for (int i = 0; i < 10; ++i) {
        if (m_compose_inventory_slots[i] == -1) {
            m_compose_inventory_slots[i] = item_id;
            audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
            return true;
        }
    }
    
    return false;
}

bool DialogBox_MailBox::on_disable()
{
    if (m_compose_active_field != 0) {
        text_input_manager::get().end_input();
        m_compose_active_field = 0;
    }
    return true;
}
