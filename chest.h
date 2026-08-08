#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"

namespace ChestRender
{
    // Bounds measured from assets/assman_out/chest.mesh after the Cube export.
    static constexpr glm::vec3 kMeshCenter(-0.16150159f, 1.06357682f, 0.33416384f);
    static constexpr float kWorldScale = 0.165f;
    static constexpr float kCollectibleWorldScale = 0.068f;
    static constexpr float kSpinRadiansPerSecond = 0.85f;
    static constexpr float kSpawnChance = 1.0f;
    static constexpr float kSpawnDelayMinSeconds = 2.0f;
    static constexpr float kSpawnDelayMaxSeconds = 6.0f;
    static constexpr float kAvailableSeconds = 5.0f;
    static constexpr float kSpinInSeconds = 0.25f;
    static constexpr float kSpinOutSeconds = 0.25f;
    static constexpr float kCollectMoveSeconds = 0.52f;
    static constexpr float kAlignToFaceSeconds = 0.42f;
    static constexpr float kOpenSeconds = 0.46f;
    static constexpr float kRewardCloseSeconds = 0.70f;
    static constexpr float kRewardSpinOutSeconds = 0.42f;
    static constexpr float kCoinIntervalSeconds = 0.01f;

    enum class CollectiblePhase
    {
        Disabled,
        Waiting,
        Available,
        Expiring,
        CollectedMove,
        WaitingTap,
        Aligning,
        Opening,
        Payout,
        RewardClosing,
        RewardSpinOut,
    };

    struct ChestState
    {
        float seconds = 0.0f;
        float cycleSeconds = 1.0f;

        float cycleT() const
        {
            if (cycleSeconds <= 0.0f)
                return 0.0f;
            return std::fmod(seconds, cycleSeconds) / cycleSeconds;
        }

        bool isOpening() const
        {
            return cycleT() < 0.5f;
        }

        bool isClosing() const
        {
            return !isOpening();
        }

        float howMuchOpen() const
        {
            const float t = cycleT();
            return t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f;
        }

        float clipTime(float clipDuration) const
        {
            if (clipDuration <= 0.0f)
                return 0.0f;
            return howMuchOpen() * clipDuration;
        }
    };

    inline glm::mat4 ModelAtIdleBallPosition(const glm::vec3 &idleBallPos, float seconds)
    {
        return glm::translate(glm::mat4(1.0f), idleBallPos) *
               glm::rotate(glm::mat4(1.0f), seconds * kSpinRadiansPerSecond, glm::vec3(0.0f, 1.0f, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(kWorldScale)) *
               glm::translate(glm::mat4(1.0f), -kMeshCenter);
    }

    inline glm::mat4 ModelAt(const glm::vec3 &pos, float yawRadians, float scale)
    {
        return glm::translate(glm::mat4(1.0f), pos) *
               glm::rotate(glm::mat4(1.0f), yawRadians, glm::vec3(0.0f, 1.0f, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
               glm::translate(glm::mat4(1.0f), -kMeshCenter);
    }

    inline void ApplyEverythingAtlasParams(ShaderProgram &shader)
    {
        shader.updateTextureParamsInOneGo(
            glm::vec3(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            1.0f
        );
    }

    inline float PingPongOpenCloseTime(float seconds, float clipDuration)
    {
        return ChestState{seconds, 1.0f}.clipTime(clipDuration);
    }

    inline float Smooth01(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float ScaleForAvailable(float ageSeconds)
    {
        if (ageSeconds < kSpinInSeconds)
            return Smooth01(ageSeconds / kSpinInSeconds);
        const float outStart = kAvailableSeconds - kSpinOutSeconds;
        if (ageSeconds > outStart)
            return 1.0f - Smooth01((ageSeconds - outStart) / kSpinOutSeconds);
        return 1.0f;
    }

    inline float Deterministic01(int seed)
    {
        uint32_t x = (uint32_t)seed * 747796405u + 2891336453u;
        x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        x = (x >> 22u) ^ x;
        return (float)(x & 0x00FFFFFFu) / (float)0x01000000u;
    }
}
