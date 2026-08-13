#pragma once

#include "ServerCommand.h"

class CmdStartCrusade : public ServerCommand
{
public:
	const char* get_name() const override { return "startcrusade"; }
	const char* GetDescription() const override { return "Manually starts a crusade"; }
	const char* GetHelp() const override { return "Usage: startcrusade"; }
	void execute(CGame* game, const char* args) override;
};