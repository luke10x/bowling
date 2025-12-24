#pragma once

#include "glm/glm.hpp"

struct Transition
{
    // shoudl switch to swing
    glm::vec2 mPrevDir{0.0f};
    float mIntentTimer = 0.0f;
    bool mWantsPhysics = false;

    glm::vec2 mPrevNdc {0.0f};

bool wantsPhysics(glm::vec2 ndc, float deltaTime)
{
    const float dy = ndc.y - mPrevNdc.y;

    constexpr float kBackHoldLimit = -0.5f;   // 🔒 deep pull-back zone
    constexpr float kVelEps        = 0.001f;

    // 🔒 Still deep in pull-back → never switch yet
    if (ndc.y < kBackHoldLimit)
    {
        mIntentTimer = 0.0f;
        mWantsPhysics = false;
        mPrevNdc = ndc;
        return false;
    }

    // 🚀 Started moving forward → instant physics
    if (dy > kVelEps)
    {
        mIntentTimer = 0.0f;
        mWantsPhysics = true;
        mPrevNdc = ndc;
        return true;
    }

    // 🚫 Actively moving backward → block physics
    if (dy < -kVelEps)
    {
        mIntentTimer = 0.0f;
        mWantsPhysics = false;
        mPrevNdc = ndc;
        return false;
    }

    // Holding still → keep state
    mPrevNdc = ndc;
    return mWantsPhysics;
}
};