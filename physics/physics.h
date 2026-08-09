#pragma once

#include "../block/block.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
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
    glm::vec3 get_ball_angular_velocity() const;

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
    void explode_ball(const glm::vec3 &origin, float impulseStrength);
    void remove_ball_from_play(const glm::vec3 &origin);
    void SpawnBallShards(const glm::vec3 &origin, const glm::vec3 *velocities, int count);
    void ClearBallShards();
    bool GetBallShardMatrix(int index, glm::mat4 &outMatrix) const;

    void set_ball_mass(float mass);

    bool is_settling_started() const;

    bool was_pin_hit(int i) const;

    int get_number_of_impacts() const;
    int get_pin_pin_hit_count() const;

    void apply_lane_pushback(float peakZ, float halfWidth, float maxStrength);

    void set_lane_pushback_params(float peakZ, float halfWidth, float maxStrength, bool enabled);

    void set_lane_pushback_oil_profile(float startZ, float endZ, float maxStrength, float easeExp, bool enabled);

    void apply_friction_to_lane(float friction);

    void set_ball_friction(float friction);

    // Restitution (bounciness) tuning
    void apply_restitution_to_lane(float restitution);
    void set_ball_restitution(float restitution);
    void set_pins_restitution(float restitution);

    // Pin tuning
    void set_pins_friction(float friction);
    void set_pins_mass(float mass);
    void set_pin_freeze_mask(uint16_t frozenMask);
    uint16_t get_pin_freeze_mask() const;
    uint16_t consume_direct_ball_pin_hit_mask();

    void set_pending_release_angular_velocity(const glm::vec3 &angVel);

    void add_ball_angular_velocity(const glm::vec3 &angVel);

    void set_ball_rotation(const glm::quat &rot);

    void apply_spin_curve();

    void set_spin_speed(float spinSpeed);

    void apply_angular_velocity_on_ball(float spinSpeed);

    void apply_pending_spin_kicks();

    int checkThrowComplete(float stillThreshold, float floorY);

    // Non-destructive query: counts pins that are "down" by position/orientation,
    // without waiting for settling and without mutating mPinDead.
    int estimatePinsDown(float floorY, float standingDotThreshold = 0.85f) const;

    void count_masters_begin_pin_crash(
        const glm::vec2 *malachPositions,
        int malachCount,
        glm::vec2 velocity,
        const glm::vec3 *pinPositions,
        float malachRadius,
        float malachHalfHeight,
        float laneHalfWidth,
        float laneEndZ
    );
    void count_masters_clear_pin_crash();
    void count_masters_query_pin_crash(
        float floorY,
        float laneHalfWidth,
        float maxZ,
        int *outPinsDown,
        int *outMalachimAlive,
        glm::vec2 *outMalachPositions,
        int maxMalachPositions
    ) const;
    int count_masters_query_falling_malach_matrices(
        glm::mat4 *outMatrices,
        int maxMatrices,
        float floorY
    ) const;
    int count_masters_query_active_malach_matrices(
        glm::mat4 *outMatrices,
        int maxMatrices,
        float floorY
    ) const;

    int get_lane_hit_count() const;

    void GenerateFracturedBlock(
        const FracturedBlockSettings &settings,
        std::vector<FracturedBlockFragmentGeometry> *outFragments = nullptr
    );
    bool ExplodeFracturedBlock(const glm::vec3 &origin, float impulseStrength, float radius);
    void ClearFracturedBlock();
    bool HasFracturedBlock() const;
    bool IsFracturedBlockBroken() const;
    int GetFracturedBlockFragmentCount() const;
    int GetFracturedBlockVariantIndex() const;
    int GetFracturedBlockBallContactCount() const;
    int GetFracturedBlockBallFirstContactCount() const;
    int GetFracturedBlockFragmentLaneHitCount() const;
    bool GetFracturedBlockFragmentMatrix(int index, glm::mat4 &outMatrix) const;
};
