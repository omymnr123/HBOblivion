#pragma once
#include "IDialogBox.h"

class DialogBox_StatusOverlay : public IDialogBox
{
public:
	DialogBox_StatusOverlay(CGame* game);
	~DialogBox_StatusOverlay() override = default;

	void on_update() override;
	void on_draw() override;
	bool on_click() override;

	bool m_show_extraloot = false;

private:
	static constexpr int padding = 10;
	static constexpr int gap = 4;
	static constexpr int margin_right = 8;
	static constexpr int above_hud = 20;

	bool m_show_levelup = false;
	bool m_show_restart = false;

	// Button rect relative to dialog origin (x, y, w, h)
	ui_rect m_primary_btn{};

	// Botón de Game Master (El servidor valida la seguridad de nivel 1000)
	bool m_show_tester = false;
	ui_rect m_tester_btn{};

	const char* get_primary_text() const;
};