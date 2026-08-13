#pragma once

#include "ServerCommand.h"

class CmdEndCrusade : public ServerCommand
{
public:
	const char* get_name() const override { return "endcrusade"; }
	const char* GetDescription() const override { return "Manually ends a crusade"; }
	const char* GetHelp() const override { return "Usage: endcrusade [winner_side] (0=Draw, 1=Aresden, 2=Elvine)"; }
	void execute(CGame* game, const char* args) override;
};