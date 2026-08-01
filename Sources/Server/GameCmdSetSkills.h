#pragma once
#include "GameChatCommand.h"

class GameCmdSetSkills : public GameChatCommand
{
public:
    const char* get_name() const override { return "setskills"; }
    int get_default_level() const override { return 1000; } // Solo administradores nivel 1000
    bool execute(CGame* game, int client_h, const char* args) override;
};