#pragma once
#include "IDialogBox.h"
#include "Packet/PacketNotify.h"
#include <string>

class DialogBox_NpcCreator : public IDialogBox
{
public:
	DialogBox_NpcCreator(class CGame* game);
	~DialogBox_NpcCreator() override = default;

	void on_draw() override;
	bool on_click() override;
	bool on_disable() override;
	bool on_enable(int type, int64_t v1, int v2, const char* string) override;

	void receive_search_results(const hb::net::PacketNotifyGameMasterNpcSearchResult* pkt);
	void on_enter_pressed();

private:
	// Search state
	std::string m_search_text;
	std::string m_last_sent_search;
	int m_result_count = 0;
	hb::net::GameMasterNpcSearchEntry m_results[200]{};
	int m_selected_index = -1;
	int m_scroll_offset = 0;
	bool m_initial_load = false;

	// Track dialog position to fix cursor drift
	short m_last_sx = 0;
	short m_last_sy = 0;

	// Dropdown state
	int m_item_count = 1;       // 1-100
	bool m_count_dropdown_open = false;
	int m_dropdown_scroll = 0;

	void draw_dropdown_field(int x, int y, int w, const char* text, bool is_open, bool is_hover);
};
