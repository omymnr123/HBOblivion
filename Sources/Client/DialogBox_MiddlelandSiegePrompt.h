#pragma once

#include "IDialogBox.h"
#include <string>

class CGame;

class DialogBox_MiddlelandSiegePrompt : public IDialogBox
{
public:
	DialogBox_MiddlelandSiegePrompt(CGame* game);
	~DialogBox_MiddlelandSiegePrompt() override = default;

	void on_draw() override;
	bool on_click() override;

	bool on_enable(int type, int64_t v1, int v2, const char* string) override;
	bool on_disable() override;
};
