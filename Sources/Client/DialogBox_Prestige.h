#pragma once
#include "IDialogBox.h"

class DialogBox_Prestige : public IDialogBox
{
public:
	DialogBox_Prestige(CGame* game);
	~DialogBox_Prestige() override = default;

	void on_draw() override;
	bool on_click() override;
};