#pragma once

#include <cmath>

namespace LanePushback
{
// Match the playable lane collider and its rendered surface width.
constexpr float PLAYABLE_HALF_WIDTH_M = 0.5f * (41.857f * 0.0254f) + 0.02f;
constexpr float BALL_RADIUS_M = 0.11f;
// Stop lane forces before the ball's center reaches the lip, while most of
// its contact patch is still supported by the lane.
constexpr float SUPPORTED_HALF_WIDTH_M = PLAYABLE_HALF_WIDTH_M - BALL_RADIUS_M * 0.5f;
constexpr float MIN_SURFACE_CENTER_Y_M = 0.06f;
constexpr float MAX_SURFACE_CENTER_Y_M = 0.15f;

inline bool IsBallOnPlayableLane(float centerX, float centerY)
{
    return std::isfinite(centerX) && std::isfinite(centerY) &&
           centerY >= MIN_SURFACE_CENTER_Y_M && centerY < MAX_SURFACE_CENTER_Y_M &&
           fabsf(centerX) <= SUPPORTED_HALF_WIDTH_M;
}
} // namespace LanePushback
