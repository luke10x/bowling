#pragma once

#include "glm/glm.hpp"

struct Transition {
    // shoudl switch to swing
    glm::vec2 mPrevDir {0.0f};
    float     mIntentTimer = 0.0f;
    bool      mWantsPhysics = false;

    bool wantsPhysics(glm::vec2 ndc, float deltaTime) {

        ndc.y *= -1.0f;
        const float magnitude = glm::length(ndc);

        // Dead zone
        constexpr float kDeadZone = 0.1f;
        if (magnitude < kDeadZone)
        {
            this->mIntentTimer  = 0.0f;
            this->mWantsPhysics = false;
            return this->mWantsPhysics;
        }

        // Normalised direction
        glm::vec2 dir = ndc / magnitude;

        // Direction stability
        float alignment = glm::dot(dir, this->mPrevDir);

        constexpr float kStableAlignment = 0.95f;
        bool stable = alignment >= kStableAlignment;

        // Physics intent threshold
        constexpr float kEnterThreshold = 0.35f;

        if (magnitude >= kEnterThreshold && stable)
        {
            this->mIntentTimer += deltaTime;
        }
        else
        {
            this->mIntentTimer = 0.0f;
        }

        constexpr float kIntentTime = 0.12f; // seconds
        this->mWantsPhysics = this->mIntentTimer >= kIntentTime;

        // Update previous direction
        this->mPrevDir = dir;
        return this->mWantsPhysics;
    }
};