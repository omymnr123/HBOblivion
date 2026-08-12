#pragma once

#include "IDialogBox.h"
#include <string>

class CGame;

class DialogBox_GuildInvite : public IDialogBox
{
public:
	DialogBox_GuildInvite(CGame* game);
	~DialogBox_GuildInvite() override = default;

	void on_draw() override;
	bool on_click() override;
	bool on_enable(int type, int64_t v1, int v2, const char* string) override;
	bool on_disable() override;

private:
	std::string m_inviter_name;
	std::string m_guild_name;
};
