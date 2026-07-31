#pragma once
#include "ServerCommand.h"

class CmdKickPlayer : public ServerCommand
{
public:
    const char* get_name() const override { return "kick-player"; }
    const char* GetDescription() const override { return "Force logout a player ignoring movement cancellation"; }
    const char* GetHelp() const override { return "Usage: kick-player <character_name>\n  Forces the specified online player to logout immediately."; }
    void execute(CGame* game, const char* args) override;
};