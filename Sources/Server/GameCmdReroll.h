#pragma once

#include "GameChatCommand.h"

class GameCmdReroll : public GameChatCommand
{
public:
	const char* get_name() const override { return "reroll"; }
	int get_default_level() const override { return 0; }
	bool execute(CGame* game, int client_h, const char* args) override;
};
