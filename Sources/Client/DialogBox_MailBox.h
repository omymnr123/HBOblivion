#pragma once
#include "IDialogBox.h"
#include <vector>
#include <string>

class DialogBox_MailBox : public IDialogBox
{
public:
	DialogBox_MailBox(CGame* game);
	~DialogBox_MailBox() override = default;

	void on_draw() override;
	bool on_click() override;
	bool on_disable() override;
    bool on_item_drop() override;

    enum class mode : uint8_t {
        list = 0,
        read = 1,
        compose = 2
    };

    void set_mode(mode new_mode) { m_mode = new_mode; }

    mode m_mode{mode::list};

    struct MailEntry {
        uint32_t mail_id;
        std::string sender;
        std::string subject;
        bool has_attachment;
        bool is_read;
    };
    std::vector<MailEntry> m_mails;
    
    // For Read mode
    uint32_t m_read_mail_id{0};
    std::string m_read_sender;
    std::string m_read_subject;
    std::string m_read_body;
    int m_read_gold{0};
    struct ReadAttachment {
        int item_id;
        uint64_t item_count;
    };
    ReadAttachment m_read_attachments[10]{};

    // For Compose mode
    std::string m_compose_receiver;
    std::string m_compose_subject;
    std::string m_compose_body;
    int m_compose_active_field = 0; // 0=none, 1=receiver, 2=subject, 3=body
    int m_compose_inventory_slots[10];
    
    int m_scroll_offset = 0;

private:
	void DrawMode_List(short sX, short sY, short size_x);
	void DrawMode_Read(short sX, short sY, short size_x);
	void DrawMode_Compose(short sX, short sY, short size_x);

	bool OnClick_List(short sX, short sY);
	bool OnClick_Read(short sX, short sY);
	bool OnClick_Compose(short sX, short sY);
};
