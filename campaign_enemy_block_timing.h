#pragma once

#include <algorithm>

struct CampaignEnemyBlockTimingPlan
{
    bool valid = false;
    float forwardDistanceM = 0.0f;
    float etaS = -1.0f;
    float deployAtS = -1.0f;
};

inline float CampaignEnemyBlockForwardDistanceM(float ballZ, float blockZ, float laneDir)
{
    const float dir = (laneDir >= 0.0f) ? 1.0f : -1.0f;
    return (blockZ - ballZ) * dir;
}

inline CampaignEnemyBlockTimingPlan CampaignEnemyBlockMakeTimingPlan(
    float nowS,
    float ballZ,
    float blockZ,
    float ballVelocityZ,
    float laneDir,
    float warningLeadS = 0.4f
)
{
    CampaignEnemyBlockTimingPlan plan;
    const float dir = (laneDir >= 0.0f) ? 1.0f : -1.0f;
    const float forwardDistance = CampaignEnemyBlockForwardDistanceM(ballZ, blockZ, dir);
    plan.forwardDistanceM = forwardDistance;
    if (forwardDistance <= 1.0e-4f)
        return plan;

    const float forwardSpeed = ballVelocityZ * dir;
    if (forwardSpeed <= 1.0e-4f)
        return plan;

    plan.valid = true;
    plan.etaS = forwardDistance / forwardSpeed;
    plan.deployAtS = nowS + std::max(plan.etaS - std::max(warningLeadS, 0.0f), 0.0f);
    return plan;
}

