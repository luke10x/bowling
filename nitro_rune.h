#pragma once

#include <algorithm>

struct NitroRuneTuning
{
    static constexpr float UseBelowCharge01 = 0.80f;
    static constexpr float RefillSeconds = 0.85f;
    static constexpr float BlinkSeconds = 1.25f;
};

static inline bool NitroRune_CanRefill(float charge, float capacity)
{
    return capacity > 0.0f && charge / capacity < NitroRuneTuning::UseBelowCharge01;
}

static inline float NitroRune_EasedRefill01(float elapsed)
{
    const float t = std::clamp(elapsed / NitroRuneTuning::RefillSeconds, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}
