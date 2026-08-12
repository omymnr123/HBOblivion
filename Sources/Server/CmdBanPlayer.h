#pragma once
#include "ServerCommand.h"

class CmdBanPlayer : public ServerCommand
{
public:
    const char* get_name() const override { return "ban-player"; }
    const char* GetDescription() const override { return "Temporarily or permanently ban a player account"; }
    const char* GetHelp() const override { return "Usage: ban-player <character_name> <duration_in_seconds>\n  Bans the account associated with the character for the specified duration."; }
    void execute(CGame* game, const char* args) override;
};