#pragma once
#include "ServerCommand.h"

class CmdRestart : public ServerCommand
{
public:
	const char* get_name() const override { return "restart"; }
	const char* GetDescription() const override { return "Save all players and gracefully RESTART the server"; }
	const char* GetHelp() const override { return "Usage: restart [seconds]\n  Saves players and restarts the server to reload configs."; }
	void execute(class CGame* game, const char* args) override;
};