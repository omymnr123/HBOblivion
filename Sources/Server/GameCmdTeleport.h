#pragma once
#include "GameChatCommand.h"

class GameCmdTeleport : public GameChatCommand
{
public:
    const char* get_name() const override { return "tp"; }
    int get_default_level() const override { return 1000; }
    bool execute(CGame* game, int client_h, const char* args) override;
};