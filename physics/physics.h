#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>

struct Physics
{

    glm::mat4 mBallMatrix;
    glm::mat4 mPinMatrix[10];
    float previousDelta = 0.0f;

    // Initialise Jolt and create world + bodies
    void physics_init(
        const float *laneVerts,
        unsigned int laneVertCount,
        const unsigned int *laneIndices,
        unsigned int laneIndexCount,
        glm::vec3 *pinStart,
        glm::vec3 ballStart);

    // Run simulation step
    void physics_step(float deltaSeconds);

    // Fetch model matrices for rendering
    const glm::mat4 &physics_get_ball_matrix();
    const glm::mat4 &physics_get_pin_matrix(int i);

    // Optional: reset ball/pin positions
    void physics_reset(glm::vec3 *newPinPos, glm::vec3 newBallPos);
};