#include "DialogBox_Talent.h"
#include "Game.h"
#include "LAN_ENG.H"
#include "TextLibExt.h"
#include "GameFonts.h"
#include "IInput.h"
#include "AudioManager.h"
#include "ItemTooltip.h"
#include "Render/IRenderer.h"
#include <format>
#include <cstring>

using namespace hb::client::sprite_id;
using namespace hb::shared::net;

DialogBox_Talent::DialogBox_Talent(CGame* game)
    : IDialogBox(DialogBoxId::Guild, game) // Puedes usar un ID propio o de reserva si lo añades a las enums
    , m_talent_points(0)
{
    set_default_rect(0, 0, 258, 300); // Tamaño similar al de gremios
    m_can_close_on_right_click = true;
    std::memset(m_talents, 0, sizeof(m_talents));
}

void DialogBox_Talent::update_talent_data(int points, const uint8_t* talents)
{
    m_talent_points = points;
    if (talents) {
        std::memcpy(m_talents, talents, sizeof(m_talents));
    }
}

void DialogBox_Talent::on_draw()
{
    short sX = m_x;
    short sY = m_y;

    // Fondo de ventana estándar
    if (m_game) {
        m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 2);
    }

    // Título
    hb::shared::text::draw_text(GameFont::Default, sX + 90, sY + 15, "Talent System", hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 255, 255)));

    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

    // Puntos disponibles
    std::string pts_str = std::format("Available Points: {}", m_talent_points);
    hb::shared::text::draw_text(GameFont::Default, sX + 27, sY + 45, pts_str.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(255, 215, 0)));

    // Lista de Talentos del Guerrero (Ejemplo: Furia y Golpe Triturador)
    const char* talent_names[3] = {
        "1. Overwhelming Strength (Furia)",
        "2. Unused Talent",
        "3. Crushing Blow (Triturador)"
    };

    for (int i = 0; i < 3; ++i) {
        int lineY = sY + 80 + (i * 40);
        
        std::string info = std::format("{} (Lvl {})", talent_names[i], m_talents[i]);
        hb::shared::text::draw_text(GameFont::Default, sX + 27, lineY, info.c_str(), hb::shared::text::TextStyle::with_shadow(hb::shared::render::Color(200, 200, 200)));

        // Botón [+] para subir de nivel si hay puntos
        if (m_talent_points > 0 && m_talents[i] < 3) {
            bool hover = (mx >= sX + 210 && mx <= sX + 230 && my >= lineY && my <= lineY + 15);
            hb::shared::render::Color btn_color = hover ? hb::shared::render::Color(255, 255, 255) : hb::shared::render::Color(150, 150, 150);
            hb::shared::text::draw_text(GameFont::Default, sX + 210, lineY, "[+]", hb::shared::text::TextStyle::with_shadow(btn_color));
        }
    }
}

bool DialogBox_Talent::on_click()
{
    short sX = m_x;
    short sY = m_y;
    short mx = static_cast<short>(hb::shared::input::get_mouse_x());
    short my = static_cast<short>(hb::shared::input::get_mouse_y());

    // Comprobar clics en los botones [+] de los talentos
    if (m_talent_points > 0) {
        for (int i = 0; i < 3; ++i) {
            int lineY = sY + 80 + (i * 40);
            if (mx >= sX + 210 && mx <= sX + 230 && my >= lineY && my <= lineY + 15) {
                if (m_talents[i] < 3) {
                    // Enviar paquete de red pidiendo subir este talento usando CommonType::RequestUpgradeTalent
                    hb::net::packet_base pkt;
                    std::memset(&pkt, 0, sizeof(pkt));
                    auto* header = reinterpret_cast<hb::net::PacketHeader*>(&pkt);
                    header->msg_id = MsgId::StateChangePoint; // O tu MsgId asignado
                    header->msg_type = CommonType::RequestUpgradeTalent;
                    
                    // Añadir el índice del talento en los datos del paquete si es necesario
                    // (dependiendo de cómo estructures tus paquetes de acción)

                    send_game_packet_impl(pkt, sizeof(hb::net::PacketHeader), true);
                    audio_manager::get().play_game_sound(sound_type::effect, 14, 5);
                    return true;
                }
            }
        }
    }

    return false;
}

bool DialogBox_Talent::on_enable(int type, int64_t v1, int v2, const char* string)
{
    if (!m_has_been_opened) {
        auto* char_dlg = m_game->get_dialog_box_manager().get_dialog_box(DialogBoxId::CharacterInfo);
        if (char_dlg) {
            m_x = char_dlg->m_x + char_dlg->m_size_x + 5;
            m_y = char_dlg->m_y;
        }
        m_has_been_opened = true;
    }
    return true;
}

bool DialogBox_Talent::on_disable()
{
    return true;
}