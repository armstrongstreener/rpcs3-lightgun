#include "stdafx.h"
#include "raw_mouse_config.h"
#include "Emu/Io/MouseHandler.h"

LOG_CHANNEL(cfg_log, "CFG");

cfg::string& raw_mouse_config::get_action_label_by_index(int index)
{
	switch (index)
	{
	case 0: return action_label_1;
	case 1: return action_label_2;
	case 2: return action_label_3;
	case 3: return action_label_4;
	case 4: return action_label_5;
	case 5: return action_label_6;
	case 6: return action_label_7;
	case 7: return action_label_8;
	default: fmt::throw_exception("Invalid action label index %d", index);
	}
}

cfg::string& raw_mouse_config::get_button_by_index(int index)
{
	switch (index)
	{
	case 0: return mouse_button_1;
	case 1: return mouse_button_2;
	case 2: return mouse_button_3;
	case 3: return mouse_button_4;
	case 4: return mouse_button_5;
	case 5: return mouse_button_6;
	case 6: return mouse_button_7;
	case 7: return mouse_button_8;
	case 8: return mouse_button_1_alternate;
	case 9: return mouse_button_2_alternate;
	case 10: return mouse_button_3_alternate;
	case 11: return mouse_button_4_alternate;
	case 12: return mouse_button_5_alternate;
	case 13: return mouse_button_6_alternate;
	case 14: return mouse_button_7_alternate;
	case 15: return mouse_button_8_alternate;
	default: fmt::throw_exception("Invalid index %d", index);
	}
}

cfg::string& raw_mouse_config::get_button(int code)
{
	switch (code)
	{
	case CELL_MOUSE_BUTTON_1: return mouse_button_1;
	case CELL_MOUSE_BUTTON_2: return mouse_button_2;
	case CELL_MOUSE_BUTTON_3: return mouse_button_3;
	case CELL_MOUSE_BUTTON_4: return mouse_button_4;
	case CELL_MOUSE_BUTTON_5: return mouse_button_5;
	case CELL_MOUSE_BUTTON_6: return mouse_button_6;
	case CELL_MOUSE_BUTTON_7: return mouse_button_7;
	case CELL_MOUSE_BUTTON_8: return mouse_button_8;
	default: fmt::throw_exception("Invalid code %d", code);
	}
}

std::string raw_mouse_config::get_button_name(std::string_view value)
{
	if (raw_mouse_button_map.contains(value))
	{
		return std::string(value);
	}

	if (value.starts_with(key_prefix))
	{
		s64 scan_code{};
		if (try_to_int64(&scan_code, value.substr(key_prefix.size()), s32{smin}, s32{smax}))
		{
			return get_key_name(static_cast<s32>(scan_code));
		}
	}

	return "";
}

std::string raw_mouse_config::get_button_name(s32 button_code)
{
	for (const auto& [name, code] : raw_mouse_button_map)
	{
		if (code == button_code)
		{
			return std::string(name);
		}
	}
	return "";
}

std::string raw_mouse_config::get_key_name(s32 scan_code)
{
#ifdef _WIN32
	TCHAR name_buf[MAX_PATH]{};
	if (!GetKeyNameTextW(scan_code, name_buf, MAX_PATH))
	{
		cfg_log.error("raw_mouse_config: GetKeyNameText failed: %s", fmt::win_error{GetLastError(), nullptr});
		return {};
	}
	return wchar_to_utf8(name_buf);
#else
	static_cast<void>(scan_code);
	return "";
#endif
}

raw_mice_config::raw_mice_config()
{
	for (u32 i = 0; i < ::size32(players); i++)
	{
		players.at(i) = std::make_shared<raw_mouse_config>(this, fmt::format("Player %d", i + 1));
	}
}

bool raw_mice_config::load(const std::string& title_id)
{
	std::lock_guard lock(m_mutex);

	// Title IDs are directory names, never arbitrary paths supplied by a profile.
	if (!title_id.empty() && (title_id.size() != 9 || !std::all_of(title_id.begin(), title_id.end(), [](char c)
														  {
															  return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
														  })))
	{
		cfg_log.error("Invalid lightgun profile title ID: %s", title_id);
		return false;
	}

	m_title_id = title_id;
	from_default();
	const std::string global_name = fmt::format("%s%s.yml", fs::get_config_dir(true), cfg_id);
	bool result = true;
	if (fs::file file{global_name, fs::read})
	{
		const std::string content = file.to_string();
		result = content.empty() || from_string(content);
	}

	if (!m_title_id.empty())
	{
		const std::string profile_name = fmt::format("%sinput_configs/lightgun/%s/%s.yml", fs::get_config_dir(true), m_title_id, cfg_id);
		cfg_log.notice("Loading lightgun profile: %s", profile_name);
		if (fs::file file{profile_name, fs::read})
		{
			const std::string inherited = to_string();
			if (const std::string content = file.to_string(); !content.empty() && !from_string(content))
			{
				from_string(inherited);
				cfg_log.error("Invalid lightgun profile: %s. Using global bindings.", profile_name);
				result = false;
			}
		}
	}

	reload_requested = true;
	return result;
}

void raw_mice_config::save()
{
	std::lock_guard lock(m_mutex);

	const std::string cfg_name = m_title_id.empty() ? fmt::format("%s%s.yml", fs::get_config_dir(true), cfg_id) : fmt::format("%sinput_configs/lightgun/%s/%s.yml", fs::get_config_dir(true), m_title_id, cfg_id);
	cfg_log.notice("Saving %s config to '%s'", cfg_id, cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	if (!cfg::node::save(cfg_name))
	{
		cfg_log.error("Failed to save %s config to '%s' (error=%s)", cfg_id, cfg_name, fs::g_tls_error);
	}

	reload_requested = true;
}
