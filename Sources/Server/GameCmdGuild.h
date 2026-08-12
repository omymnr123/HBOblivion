#pragma once

#include "GameChatCommand.h"

class GameCmdGuild : public GameChatCommand
{
public:
	const char* get_name() const override { return "guild"; }
	int get_default_level() const override { return 0; }
	bool requires_gm_mode() const override { return false; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
