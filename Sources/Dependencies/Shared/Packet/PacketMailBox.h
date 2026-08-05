#pragma once
#include <cstdint>
#include "PacketHeaders.h"

namespace hb {
namespace net {

#pragma pack(push, 1)

// 1. El Cliente pide la lista de correos al entrar al juego (Solo cabecera)
struct PacketRequestMailList : packet_base {
    PacketHeader header; // msg_id = MsgId::RequestMailList
};

// 2. El Cliente envía un correo nuevo
struct PacketRequestSendMail : packet_base {
    PacketHeader header; // msg_id = MsgId::RequestSendMail
    char receiver_name[21]; // Nombre del destinatario
    char subject[40];       // Asunto del correo
    char body[200];         // Texto del mensaje
    uint32_t attached_gold; // Cantidad de oro adjunto
    int16_t inventory_slot; // Slot del inventario del ítem (-1 si no envía nada)
};

// 3. El Cliente pide leer un correo específico
struct PacketRequestReadMail : packet_base {
    PacketHeader header; // msg_id = MsgId::RequestReadMail
    uint32_t mail_id;    // ID único del correo en la base de datos
};

// 4. El Cliente reclama el objeto/oro de un correo
struct PacketRequestTakeAttachment : packet_base {
    PacketHeader header; // msg_id = MsgId::RequestTakeAttachment
    uint32_t mail_id;
};

// 5. El Cliente borra un correo
struct PacketRequestDeleteMail : packet_base {
    PacketHeader header; // msg_id = MsgId::RequestDeleteMail
    uint32_t mail_id;
};
// --- RESPUESTAS DEL SERVIDOR AL CLIENTE ---

// Estructura de un solo correo resumido para la lista
struct MailListEntry {
    uint32_t mail_id;
    char sender_name[21];
    char subject[40];
    uint8_t has_attachment; // 1 si tiene oro/item, 0 si no
    uint8_t is_read;        // 1 si está leído, 0 si no
};

// El sobre que contiene la lista entera de correos (hasta 20 de golpe)
struct PacketResponseMailList : packet_base {
    PacketHeader header; // msg_id = MsgId::ResponseMailList
    uint16_t mail_count;
    MailListEntry mails[20]; 
};

// El sobre que contiene el texto y datos de un correo específico al abrirlo
struct PacketResponseReadMail : packet_base {
    PacketHeader header; // msg_id = MsgId::ResponseReadMail
    uint32_t mail_id;
    char body[200];
    uint32_t attached_gold;
    int attached_item_id;
    uint64_t attached_item_count;
};

#pragma pack(pop)

}
}