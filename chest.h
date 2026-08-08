#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "mesh.h"

namespace ChestRender
{
    // Bounds measured from assets/assman_out/chest.mesh after the Cube export.
    static constexpr glm::vec3 kMeshCenter(-0.16150159f, 1.06357682f, 0.33416384f);
    static constexpr float kWorldScale = 0.11f;
    static constexpr float kSpinRadiansPerSecond = 0.85f;

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

}
