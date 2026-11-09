#pragma once
#include <cstdint>
#include <cmath>
#include <array>

// One complete turn per press; holding the input does not repeat it.
struct lightgun_spin
{
    static constexpr std::uint64_t duration_us = 1'200'000;
    // Horizontal arm orbit, in millimetres, relative to the starting sphere.
    static constexpr float orbit_radius_mm = 600.f;
    static std::array<float, 2> orbit(float angle)
    {
        return {orbit_radius_mm * std::sin(angle), orbit_radius_mm * (std::cos(angle) - 1.f)};
    }
    bool held = false;
    bool active = false;
    std::uint64_t started = 0;
    std::uint64_t last_time = 0;

    float update(std::uint64_t now, bool pressed)
    {
        if (now < last_time) { active = false; held = pressed; }
        last_time = now;
        if (pressed && !held && !active) { active = true; started = now; }
        held = pressed;
        if (!active) return 0.f;
        const auto elapsed = now - started;
        if (elapsed >= duration_us) { active = false; return 0.f; }
        return 6.283185307179586f * static_cast<float>(elapsed) / duration_us;
    }
};

