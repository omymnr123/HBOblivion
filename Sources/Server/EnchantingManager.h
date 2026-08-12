// EnchantingManager.h
#pragma once

#include "CommonTypes.h"
#include <vector>
#include <memory>
#include <cstdint>

class CGame;
class CClient;
class CItem;

class EnchantingManager
{
public:
    static EnchantingManager& GetInstance()
    {
        static EnchantingManager instance;
        return instance;
    }

    // Inicialización y actualización de UI
    void SyncEnchantBag(int client_h, CClient* client);
    void SyncRecoverQueue(int client_h, CClient* client);

    // Acciones principales (Llamadas desde Game.cpp cuando llega el paquete)
    bool OnDisenchantItem(int client_h, CGame* game, int inventory_slot);
    bool OnEnchantItem(int client_h, CGame* game, int inventory_slot, int target_stat_id);
    bool OnRecoverItem(int client_h, CGame* game, int queue_db_id);
    bool OnDepositMaterials(int client_h, CGame* game, int inventory_slot); // Opcional, si los materiales se caen como items
    bool OnUpgradeMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level);
    bool OnUpgradeMaterialAll(int client_h, CGame* game, int material_type, int stat_id, int current_level);
    bool OnWithdrawMaterial(int client_h, CGame* game, int material_type, int stat_id, int current_level);

private:
    EnchantingManager() = default;
    ~EnchantingManager() = default;

    EnchantingManager(const EnchantingManager&) = delete;
    EnchantingManager& operator=(const EnchantingManager&) = delete;

    int GetDisenchantShardYield(CItem* item);
    int GetDisenchantFragmentYield(CItem* item);
    int CalculateEnchantSuccessChance(int skill_mastery);
    int GetRequiredLevelForUpgrade(int value);
};
