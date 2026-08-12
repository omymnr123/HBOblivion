// DialogBox_Enchanting.h
#pragma once

#include "IDialogBox.h"
#include <vector>

class CGame;

class DialogBox_Enchanting : public IDialogBox
{
public:
    DialogBox_Enchanting(CGame* game);
    ~DialogBox_Enchanting() override = default;

    void on_draw() override;
    bool on_click() override;
    PressResult on_press() override;
    bool on_item_drop() override;
    bool on_enable(int type, int64_t v1, int v2, const char* string) override;
    bool on_disable() override;
    bool is_draggable() const override { return true; }

private:
    void draw_left_panel(short sX, short sY, short mouse_x, short mouse_y);

    int m_active_tab;    // 0 = Enchant, 1 = Disenchant, 2 = Recover
    int m_target_item;   // Inventory index of the dropped item (-1 if empty)

    // Layout constants
    static constexpr int PANEL_W  = 260;
    static constexpr int PANEL_H  = 370;

    // Item slot area (relative to left panel)
    static constexpr int SLOT_X = 75; // Centered
    static constexpr int SLOT_Y = 135;
    static constexpr int SLOT_W = 110;
    static constexpr int SLOT_H = 100;

    // Button positions (relative to left panel)
    static constexpr int ACTION_BTN_X = 55;
    static constexpr int ACTION_BTN_Y = 295;
    static constexpr int ACTION_BTN_W = 150;
    static constexpr int ACTION_BTN_H = 20;

    static constexpr int BAG_BTN_X = 70;
    static constexpr int BAG_BTN_Y = 345;
    static constexpr int BAG_BTN_W = 120;
    static constexpr int BAG_BTN_H = 16;
};

class DialogBox_EnchantingBag : public IDialogBox
{
public:
    DialogBox_EnchantingBag(CGame* game);
    ~DialogBox_EnchantingBag() override = default;

    void on_draw() override;
    bool on_click() override;
    bool on_item_drop() override;
    bool is_draggable() const override { return true; }

private:
    int get_bag_amount(int material_type, int stat_id, int level) const;
    int m_active_tab = 0; // 0 = Shards, 1 = Fragments

    // Layout constants
    static constexpr int PANEL_W = 480;
    static constexpr int PANEL_H = 370;

    // Popup state
    bool m_show_popup = false;
    int m_popup_stat_id = 0;
    int m_popup_level = 0;
    int m_popup_tab = 0; // 0 for Shards, 1 for Fragments
    int m_popup_x = 0;
    int m_popup_y = 0;
    std::string m_popup_title;
};
