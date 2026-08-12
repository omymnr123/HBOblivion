#pragma once
#include "IDialogBox.h"
#include "DialogBoxIDs.h"
#include "GlobalDef.h"
#include "Item/ItemInstanceData.h"
#include <vector>

struct ExtraLootEntry {
    uint32_t db_id;
    int item_id;
    hb::shared::item::item_instance_data item_data;
};

class DialogBox_ExtraLoot : public IDialogBox
{
public:
    DialogBox_ExtraLoot(CGame* game);
    ~DialogBox_ExtraLoot() override = default;

    void on_update() override;
    void on_draw() override;
    bool on_click() override;
    bool on_enable(int type, int64_t v1, int v2, const char* string) override;

    void clear_loot();
    void add_loot(uint32_t db_id, int item_id, const hb::shared::item::item_instance_data& data);

private:
    std::vector<ExtraLootEntry> m_loot_list;
    void draw_tooltip(short mouse_x, short mouse_y, int index);
};