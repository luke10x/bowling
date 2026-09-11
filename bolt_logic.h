#pragma once

#include "campaign_enemy_block_timing.h"

enum class BoltRollingResolution
{
    Destroy,
    Escape,
};

inline bool BoltShouldTargetBlock(
    bool blockPresent,
    bool playerBallRolling,
    float ballZ,
    float blockZ,
    float laneDirection,
    float aheadToleranceM = 0.08f
)
{
    if (!blockPresent)
        return false;
    if (!playerBallRolling)
        return true;
    return CampaignEnemyBlockForwardDistanceM(ballZ, blockZ, laneDirection) > aheadToleranceM;
}

inline bool BoltShouldRetargetPins(bool flashActive, float flashAgeS, float strikeDurationS)
{
    return flashActive && flashAgeS < strikeDurationS;
}

inline BoltRollingResolution BoltResolveRollingBall(bool leftLaneDuringFlash, bool onLaneNow)
{
    return (leftLaneDuringFlash || !onLaneNow)
        ? BoltRollingResolution::Escape
        : BoltRollingResolution::Destroy;
}
