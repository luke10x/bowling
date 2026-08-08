#pragma once

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"

namespace ChestRender
{
    // Bounds measured from assets/assman_out/chest.mesh after the Cube export.
    static constexpr glm::vec3 kMeshCenter(-0.16150159f, 1.06357682f, 0.33416384f);
    static constexpr float kWorldScale = 0.165f;
    static constexpr float kSpinRadiansPerSecond = 0.85f;

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
}
