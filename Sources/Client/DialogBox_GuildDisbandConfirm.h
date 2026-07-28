#pragma once

#include "IDialogBox.h"

class DialogBox_GuildDisbandConfirm : public IDialogBox
{
public:
	explicit DialogBox_GuildDisbandConfirm(CGame* game);
	~DialogBox_GuildDisbandConfirm() override = default;

	void on_draw() override;
	bool on_click() override;

private:
	static constexpr ui_rect btn_yes{ 30, 55, 70, 20 };
	static constexpr ui_rect btn_no{ 170, 55, 70, 20 };
};
