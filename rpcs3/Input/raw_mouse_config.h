#pragma once

#include "Utilities/Config.h"
#include "Utilities/mutex.h"

#include <array>

#ifdef _WIN32
#include <windows.h>
#endif

static const std::map<std::string_view, int> raw_mouse_button_map{
	{"", 0},
#ifdef _WIN32
	{"Button 1", RI_MOUSE_BUTTON_1_UP},
	{"Button 2", RI_MOUSE_BUTTON_2_UP},
	{"Button 3", RI_MOUSE_BUTTON_3_UP},
	{"Button 4", RI_MOUSE_BUTTON_4_UP},
	{"Button 5", RI_MOUSE_BUTTON_5_UP},
#endif
};

struct raw_mouse_config : cfg::node
{
public:
	using cfg::node::node;

	cfg::string device{this, "Device", ""};
	cfg::_bool shoot_spin_360{this, "The Shoot Spin 360", false, true};
	cfg::_bool shoot_point_down{this, "The Shoot Point Down", false, true};
	cfg::_bool shoot_point_up_offscreen{this, "The Shoot Point Up Offscreen", false, true};
	cfg::_bool shoot_offscreen_middle{this, "The Shoot Offscreen Middle", false, true};

	cfg::_float<10, 1000> mouse_acceleration{this, "Mouse Acceleration", 100.0f, true};
	cfg::_float<10, 200> move_aim_scale_h{this, "Move Aim Scale Horizontal", 100.0f, true};
	cfg::_float<10, 200> move_aim_scale_v{this, "Move Aim Scale Vertical", 100.0f, true};

	cfg::string mouse_button_1{this, "Button 1", "Button 1", true};
	cfg::string mouse_button_2{this, "Button 2", "Button 2", true};
	cfg::string mouse_button_3{this, "Button 3", "Button 3", true};
	cfg::string mouse_button_4{this, "Button 4", "Button 4", true};
	cfg::string mouse_button_5{this, "Button 5", "Button 5", true};
	cfg::string mouse_button_6{this, "Button 6", "", true};
	cfg::string mouse_button_7{this, "Button 7", "", true};
	cfg::string mouse_button_8{this, "Button 8", "", true};

	cfg::string mouse_button_1_alternate{this, "Button 1 Alternate", "", true};
	cfg::string mouse_button_2_alternate{this, "Button 2 Alternate", "", true};
	cfg::string mouse_button_3_alternate{this, "Button 3 Alternate", "", true};
	cfg::string mouse_button_4_alternate{this, "Button 4 Alternate", "", true};
	cfg::string mouse_button_5_alternate{this, "Button 5 Alternate", "", true};
	cfg::string mouse_button_6_alternate{this, "Button 6 Alternate", "", true};
	cfg::string mouse_button_7_alternate{this, "Button 7 Alternate", "", true};
	cfg::string mouse_button_8_alternate{this, "Button 8 Alternate", "", true};

	cfg::string action_label_1{this, "Action 1 Label", "", true};
	cfg::string action_label_2{this, "Action 2 Label", "", true};
	cfg::string action_label_3{this, "Action 3 Label", "", true};
	cfg::string action_label_4{this, "Action 4 Label", "", true};
	cfg::string action_label_5{this, "Action 5 Label", "", true};
	cfg::string action_label_6{this, "Action 6 Label", "", true};
	cfg::string action_label_7{this, "Action 7 Label", "", true};
	cfg::string action_label_8{this, "Action 8 Label", "", true};

	cfg::string& get_action_label_by_index(int index);
	cfg::string& get_button_by_index(int index);
	cfg::string& get_button(int code);

	static constexpr std::string_view key_prefix = "Key ";

	static std::string get_button_name(std::string_view value);
	static std::string get_button_name(s32 button_code);
	static std::string get_key_name(s32 scan_code);
};

struct raw_mice_config : cfg::node
{
	raw_mice_config();

	shared_mutex m_mutex;
	static constexpr std::string_view cfg_id = "raw_mouse";
	std::array<std::shared_ptr<raw_mouse_config>, 4> players;
	atomic_t<bool> reload_requested = false;

	// Empty title ID selects global settings; missing game profiles inherit them.
	bool load(const std::string& title_id = {});
	void save();

private:
	std::string m_title_id;
};

extern raw_mice_config g_cfg_raw_mouse;


