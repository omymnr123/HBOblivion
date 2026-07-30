#pragma once

#include "IDialogBox.h"
#include <cstdint>

class DialogBox_Talent : public IDialogBox
{
public:
    DialogBox_Talent(CGame* game);
    ~DialogBox_Talent() override = default;

    void on_draw() override;
    bool on_click() override;

    bool on_enable(int type, int64_t v1, int v2, const char* string) override;
    bool on_disable() override;

    void update_talent_data(int points, const uint8_t* talents);

private:
    int m_talent_points;
    uint8_t m_talents[8];
    bool m_has_been_opened = false;
};