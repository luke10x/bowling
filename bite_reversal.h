#pragma once

#include <algorithm>
#include <cmath>

namespace BiteReversal
{
    // Low-bite balls need twice the counter-spin input for the same base
    // reversal. High-bite balls also turn existing sideways motion into a
    // bounded assist, so even a modest opposite spin can pull them back.
    static constexpr float kLowBiteInputGain = 0.5f;
    static constexpr float kBiteDriveFromLateralVelocity = 5.0f;
    static constexpr float kMaxAssistRelativeToInput = 2.0f;

    inline float SideDrive(float baseSideDrive, float lateralVelocity, float bite)
    {
        if (!std::isfinite(baseSideDrive) || !std::isfinite(lateralVelocity) ||
            std::fabs(baseSideDrive) <= 1e-6f || std::fabs(lateralVelocity) <= 0.02f ||
            (baseSideDrive > 0.0f) == (lateralVelocity > 0.0f))
            return baseSideDrive;

        const float bite01 = std::clamp(bite, 0.0f, 1.0f);
        const float inputGain = kLowBiteInputGain + (1.0f - kLowBiteInputGain) * bite01;
        const float biteAssist = bite01 * bite01 * bite01 * bite01;
        const float assist = std::min(
            biteAssist * kBiteDriveFromLateralVelocity * std::fabs(lateralVelocity),
            kMaxAssistRelativeToInput * std::fabs(baseSideDrive));
        return std::copysign(std::fabs(baseSideDrive) * inputGain + assist, baseSideDrive);
    }
}
