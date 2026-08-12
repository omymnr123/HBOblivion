#pragma once
#include "IDialogBox.h"

class DialogBox_Bank : public IDialogBox
{
public:
	DialogBox_Bank(CGame* game);
	~DialogBox_Bank() override = default;

	void on_draw() override;
	bool on_click() override;
	bool on_double_click() override;
	PressResult on_press() override;
	bool on_item_drop() override;

	bool on_enable(int type, int64_t v1, int v2, const char* string) override;
	bool on_disable() override;

	enum class mode : int8_t
	{
		waiting = -1,
		list = 0,
	};

	mode m_mode{mode::list};
	short m_scroll_offset{};
	int m_item_count{};
	int m_current_page{0};       // Página actual del banco (0 = I, 9 = X)
	int m_items_per_page{30};    // 30 objetos por página

private:
	void draw_item_list(short sX, short sY, short size_x);
	void draw_item_details(short sX, short sY, short size_x, int item_index, int yPos);
	void draw_scrollbar(short sX, short sY, int total_lines);
	void draw_roman_numerals(short sX, short sY, short size_x);

	static constexpr ui_rect area_scroll{240, 40, 21, 281};
};