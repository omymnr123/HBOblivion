#pragma once
#include "GameChatCommand.h"

class GameCmdWho : public GameChatCommand
{
public:
    const char* get_name() const override { return "who"; }
    
    // Nivel 0 para que todos los jugadores (Neutrales, Aresden, Elvine) puedan usarlo
    int get_default_level() const override { return 0; } 
    
    bool execute(CGame* game, int client_h, const char* args) override;
};