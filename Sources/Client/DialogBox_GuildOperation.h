#pragma once
#include "IDialogBox.h"
#include <string>

class DialogBox_GuildOperation : public IDialogBox
{
public:
	DialogBox_GuildOperation(CGame* game);
	~DialogBox_GuildOperation() override = default;

	void on_draw() override;
	bool on_click() override;

	bool on_enable(int type, int64_t v1, int v2, const char* string) override;
	bool on_disable() override;

private:
    std::string m_guild_name;
};
