#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

struct Physics
{
    glm::mat4 mBallMatrix;
    glm::mat4 mPinMatrix[10];
    bool mPinDead[10];
    float previousDelta = 0.0f;

    // Initialise Jolt and create world + bodies
    void physics_init(
        const float *laneVerts, unsigned int laneVertCount, const unsigned int *laneIndices,
        unsigned int laneIndexCount, glm::vec3 *pinStart, glm::vec3 ballStart
    );

    // Run simulation step
    void physics_step(float deltaSeconds, float physicsInterval);

    // Fetch model matrices for rendering
    const glm::mat4 &physics_get_ball_matrix();
    const glm::mat4 &physics_get_pin_matrix(int i);

    // Optional: reset ball/pin positions
    void physics_reset(glm::vec3 *newPinPos, glm::vec3 newBallPos, bool reviveAll);

    // Set manual ball position (for AIM phase)
    void set_manual_ball_position(const glm::vec3 &pos, const glm::quat &rot, float dt);

    void set_ball_hanging(const glm::vec3 pivotPoint, const glm::vec3 initialBallPos);

    void change_pivot_point(glm::vec3 newPivot);

    void set_ball_free();

    // Switch ball to physics control (start THROW phase)
    void enable_physics_on_ball();

    // Optional: store whether physics is active
    bool is_ball_physics_active() const;

    glm::vec3 get_ball_swing_movement() const;

    void set_ball_swing_movement(glm::vec3 vel);

    void set_ball_mass(float mass);

    bool is_settling_started() const;

    bool was_pin_hit(int i) const;

    int get_number_of_impacts() const;

    void apply_lane_pushback(float peakZ, float halfWidth, float maxStrength);

    void set_lane_pushback_params(float peakZ, float halfWidth, float maxStrength, bool enabled);

    void set_lane_pushback_oil_profile(float startZ, float endZ, float maxStrength, float easeExp, bool enabled);

    void apply_friction_to_lane(float friction);

    void set_ball_friction(float friction);

    void set_pending_release_angular_velocity(const glm::vec3 &angVel);

    void add_ball_angular_velocity(const glm::vec3 &angVel);

    void set_ball_rotation(const glm::quat &rot);

    void apply_spin_curve();

    void set_spin_speed(float spinSpeed);

    void apply_angular_velocity_on_ball(float spinSpeed);

    void apply_pending_spin_kicks();

    int checkThrowComplete(float stillThreshold, float floorY);
};
