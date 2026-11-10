#pragma once
#include "yaml-cpp/yaml.h"
#include <string>

// Hardware assignments belong only to config/raw_mouse.yml.
inline void strip_raw_mouse_profile_devices(YAML::Node& profile)
{
    if (!profile.IsMap()) return;
    for (unsigned i = 1; i <= 4; ++i)
    {
        auto player = profile["Player " + std::to_string(i)];
        if (player.IsMap()) player.remove("Device");
    }
}

