#pragma once

#include "IDialogBox.h"
#include "../Dependencies/Shared/Packet/PacketGuildSystem.h"

class DialogBox_Guild : public IDialogBox
{
public:
	DialogBox_Guild(CGame* game);
	~DialogBox_Guild() override = default;

	void on_draw() override;
	bool on_click() override;

	bool on_enable(int type, int64_t v1, int v2, const char* string) override;
	bool on_disable() override;

	void update_members(const hb::shared::net::PacketGuildMemberList& pkt);

private:
	int m_active_tab = 0; // 0 = Members, 1 = Economy, 2 = Passives
	int m_selected_member = -1;
	int m_sort_method = 0; // 0=Name, 1=Rank, 2=Map, 3=Status
	int m_current_page = 0;
	hb::shared::net::PacketGuildMemberList m_guild_data;
	bool m_has_been_opened = false;
	uint32_t m_last_update_time = 0;
	
	void sort_members();
	int get_my_guild_rank() const;
public:
	const hb::shared::net::PacketGuildMemberList& get_guild_data() const { return m_guild_data; }
};
