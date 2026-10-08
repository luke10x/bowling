#pragma once

#include <cmath>

namespace LanePushback
{
// Match the playable lane collider and its rendered surface width.
constexpr float PLAYABLE_HALF_WIDTH_M = 0.5f * (41.857f * 0.0254f) + 0.02f;
constexpr float MAX_SURFACE_CENTER_Y_M = 0.25f;

inline bool IsBallOnPlayableLane(float centerX, float centerY)
{
    return std::isfinite(centerX) && std::isfinite(centerY) &&
           centerY < MAX_SURFACE_CENTER_Y_M && fabsf(centerX) <= PLAYABLE_HALF_WIDTH_M;
}
} // namespace LanePushback
