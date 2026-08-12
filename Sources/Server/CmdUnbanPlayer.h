#pragma once
#include "ServerCommand.h"

class CmdUnbanPlayer : public ServerCommand
{
public:
    const char* get_name() const override { return "unban-player"; }
    const char* GetDescription() const override { return "Unban a player account"; }
    const char* GetHelp() const override { return "Usage: unban-player <character_name>\n  Removes any active ban from the account associated with the character."; }
    void execute(CGame* game, const char* args) override;
};