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

    // Set manual ball position (for AIM phase)
    void set_manual_ball_position(const glm::vec3 &pos,
                                           const glm::quat &rot,
                                           float dt);

    // Switch ball to physics control (start THROW phase)
    void enable_physics_on_ball();

    // Optional: store whether physics is active
    bool is_ball_physics_active() const;

    
    void apply_lane_pushback(float peakZ, float halfWidth, float maxStrength);

    void apply_spin_curve();
    void set_spin_speed(float spinSpeed);

    void apply_pending_spin_kicks();
};