#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <algorithm> // for std::clamp


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <algorithm>

// -----------------------------------------------------------------------------
// CoinFlyConfig — all tunable parameters (static inline const for hot-reload)
// -----------------------------------------------------------------------------
struct CoinFlyConfig {
    static inline const float FLY_DURATION   = 0.8f;   // seconds for fly animation
    static inline const float ARC_HEIGHT     = 50.0f;   // vertical arc offset (screen pixels)
    static inline const float START_SCALE    = 0.8f;   // scale at start of fly
    static inline const float END_SCALE      = 0.4f;   // scale at end of fly
    static inline const float PIXEL_SIZE     = 40.0f;  // base pixel size for rendering
    static inline const float TARGET_X       = 40.0f;  // HUD target X (client can override)
    static inline const float TARGET_Y       = 40.0f;  // HUD target Y (client can override)
};

// -----------------------------------------------------------------------------
// CoinFlyAnimation — 2D screen-space animation: world coin → HUD
// Fully timestep-independent: works at any FPS (1 FPS or 144 FPS)
// -----------------------------------------------------------------------------
struct CoinFlyAnimation {
    glm::vec2 startPos{};      // screen position where coin was collected
    glm::vec2 targetPos{};     // HUD destination
    float elapsed = 0.0f;      // accumulated time since start
    bool active = false;       // is this animation slot in use?
    
    // Computed each update (for rendering)
    glm::vec2 currentPos{};
    float currentScale = 1.0f;
    float rotationY = 0.0f;

    static inline const float SPIN_SPEED = 18.0f; // radians per second

    void start(const glm::vec2& screenPos, const glm::vec2& target) {
        startPos = screenPos;
        targetPos = target;
        elapsed = 0.0f;
        active = true;
        currentPos = screenPos;
        currentScale = CoinFlyConfig::START_SCALE;
        rotationY = 0.0f;
    }

    // ✅ TIMESTEP-SAFE: No clamping, no frame-dependency
    // Works correctly even if deltaTime is 0.1s (10 FPS) or 0.5s (2 FPS)
    void update(float deltaTime) {
        if (!active) return;
        
        elapsed += deltaTime;  // Accumulate real time
        const float duration = CoinFlyConfig::FLY_DURATION;
        
        // Normalized progress [0,1], clamped
        float t = elapsed / duration;
        if (t >= 1.0f) {
            // Animation complete: snap to target, deactivate
            currentPos = targetPos;
            currentScale = CoinFlyConfig::END_SCALE;
            active = false;  // Slot freed for reuse
            return;
        }

        // Smooth ease-in-out cubic (independent of frame rate)
        const float ease = (t < 0.5f) 
            ? 4.0f * t * t * t 
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;

        // Interpolate position with optional arc
        currentPos = glm::mix(startPos, targetPos, ease);
        if (CoinFlyConfig::ARC_HEIGHT != 0.0f) {
            currentPos.y += std::sin(t * 3.14159265f) * CoinFlyConfig::ARC_HEIGHT;
        }

        // Interpolate scale
        currentScale = glm::mix(CoinFlyConfig::START_SCALE, CoinFlyConfig::END_SCALE, ease);
        
        // Continuous spin (timestep-independent: radians = speed * seconds)
        rotationY += SPIN_SPEED * deltaTime;
    }

    [[nodiscard]] bool isComplete() const noexcept { return !active; }
};

// -----------------------------------------------------------------------------
// Coin state machine — minimal, unambiguous states
// -----------------------------------------------------------------------------
enum class CoinState : uint8_t {
    Active,      // Moving in world, can be collected
    Collected,   // Picked up, waiting for client to spawn fly animation
    Dead         // Removed from simulation (no rendering, no logic)
};

// -----------------------------------------------------------------------------
// Coin movement patterns
// -----------------------------------------------------------------------------
enum class CoinPattern : uint8_t {
    Static,      // Fixed positions in lane
    SideToSide,  // Oscillate X axis (left/right)
    WaveBop,     // Oscillate Y axis (hover up/down)
    Spiral,      // Small spiral in XZ plane
    Count
};

// -----------------------------------------------------------------------------
// Coin — single collectible object
// -----------------------------------------------------------------------------
struct Coin {
    glm::vec3 position{};        // Current world position (updated by pattern)
    glm::vec3 basePosition{};    // Rest position for pattern calculations
    glm::mat4 transform{1.0f};   // Model matrix for rendering
    
    float rotation = 0.0f;       // Y-axis rotation for visual spin
    float scale = 1.0f;          // Uniform scale factor
    float phaseOffset = 0.0f;    // Pattern timing offset (radians)
    
    CoinState state = CoinState::Dead;
    bool flyTriggered = false;   // ✅ True AFTER client successfully spawns fly animation
    
    void updateTransform() noexcept {
        transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(scale));
    }
    
    // ✅ Render in 3D only if Active and visible
    [[nodiscard]] bool isRenderable() const noexcept {
        return state == CoinState::Active && scale > 0.05f;
    }
};

// -----------------------------------------------------------------------------
// CoinLane — manages coins in a bowling lane + fly animation pool
// Interface matches your existing client code exactly
// -----------------------------------------------------------------------------
struct CoinLane {
    // === Configuration (static inline const for hot-reload) ===
    static inline const int MAX_COINS = 10;
    static inline const int MAX_FLY_ANIMATIONS = 8;
    
    // Lane geometry (world units, meters)
    static inline const float LANE_START_Z = -15.288f;  // Far end
    static inline const float LANE_END_Z   =  -1.0f;    // Near end (before pins)
    static inline const float LANE_WIDTH   =   1.054f;  // Playable width
    static inline const float GUTTER_MARGIN =  0.02f;   // Keep coins away from gutters
    
    // Collision radii
    static inline const float BALL_RADIUS = 0.108f;
    static inline const float COIN_RADIUS = 0.15f;
    static inline const float PICKUP_RADIUS_SQ = (BALL_RADIUS + COIN_RADIUS) * (BALL_RADIUS + COIN_RADIUS);
    
    // Pattern movement params
    static inline const float SIDE_AMPLITUDE = 0.70f;
    static inline const float SIDE_FREQUENCY = 0.5f;   // cycles per second
    static inline const float HOVER_AMPLITUDE = 0.03f;
    static inline const float HOVER_FREQUENCY = 3.5f;
    static inline const float SPIRAL_RADIUS = 0.12f;
    static inline const float ROTATION_SPEED = 2.5f;   // rad/s for coin visual spin
    
    // Respawn timing
    static inline const float RESPAWN_DELAY = 0.5f;    // seconds before auto-respawn
    
    // === Data ===
    std::array<Coin, MAX_COINS> coins{};
    int activeCount = 0;
    CoinPattern currentPattern = CoinPattern::Static;
    
    // ✅ Public for your render loop (matches your existing code)
    std::array<CoinFlyAnimation, MAX_FLY_ANIMATIONS> flyAnimations{};
    
    float emptyTimer = 0.0f;

    // Add this to struct CoinLane (public section):
void markFlyTriggered(int coinIndex) noexcept {
    if (coinIndex >= 0 && coinIndex < activeCount) {
        coins[coinIndex].flyTriggered = true;
    }
}
    // === Initialization ===
    void initStars(CoinPattern pattern, int count = MAX_COINS) {
        currentPattern = pattern;
        activeCount = std::clamp(count, 0, MAX_COINS);
        
        for (int i = 0; i < activeCount; ++i) {
            Coin& c = coins[i];
            
            // Distribute evenly along Z axis (lane length)
            const float t = (activeCount > 1) 
                ? static_cast<float>(i) / static_cast<float>(activeCount - 1) 
                : 0.5f;
            float z = LANE_START_Z + t * (LANE_END_Z - LANE_START_Z);
            float x = 0.0f;
            
            // Apply pattern-specific X offset
            switch (pattern) {
                case CoinPattern::SideToSide:
                    x = ((i % 2 == 0) ? -1.0f : 1.0f) * LANE_WIDTH * 0.3f;
                    break;
                case CoinPattern::WaveBop:
                case CoinPattern::Spiral:
                    x = std::sin(static_cast<float>(i) * 1.3f) * LANE_WIDTH * 0.2f;
                    break;
                default: break; // Static: x = 0
            }
            
            // Clamp to playable lane area (avoid gutters)
            x = std::clamp(x, -LANE_WIDTH * 0.5f + GUTTER_MARGIN, LANE_WIDTH * 0.5f - GUTTER_MARGIN);
            z = std::clamp(z, LANE_START_Z + 0.5f, LANE_END_Z - 0.3f);
            
            // Initialize coin
            c.basePosition = {x, 0.20f, z};  // Y = coin height above lane surface
            c.position = c.basePosition;
            c.phaseOffset = static_cast<float>(i) * 0.628f;  // ~2π/10 for staggered patterns
            c.rotation = 0.0f;
            c.scale = 1.0f;
            c.state = CoinState::Active;
            c.flyTriggered = false;
            c.updateTransform();
        }
        
        // Deactivate unused slots
        for (int i = activeCount; i < MAX_COINS; ++i) {
            coins[i].state = CoinState::Dead;
        }
        
        emptyTimer = 0.0f;
    }

    // === Per-frame update: pattern movement + collision detection ===
    // ✅ Called BEFORE client spawns fly animations
    void updateStars(const glm::vec3& prevBallPos, const glm::vec3& ballPos, 
                     float globalTime, float deltaTime) 
    {
        (void)prevBallPos;  // Reserved for continuous collision (segment test) if needed later
        
        for (int i = 0; i < activeCount; ++i) {
            Coin& c = coins[i];
            if (c.state != CoinState::Active) continue;
            
            // 1️⃣ Apply pattern movement (timestep-independent via globalTime)
            applyPattern(c, globalTime);
            
            // 2️⃣ Update visual rotation (timestep-independent)
            c.rotation += ROTATION_SPEED * deltaTime;
            c.updateTransform();
            
            // 3️⃣ Collision detection: point-in-radius (sufficient for typical bowling ball speed)
            // ✅ Fixed: removed erroneous 10x multiplier from original code
            const glm::vec3 toBall = ballPos - c.position;
            if (glm::dot(toBall, toBall) < PICKUP_RADIUS_SQ) {
                c.state = CoinState::Collected;
                c.flyTriggered = false;  // Client will set true after successful spawn
                // ⚠️ Do NOT hide coin here — client controls rendering based on flyTriggered flag
            }
        }
    }

    // === Fly animation management ===
    // ✅ Called every frame to update all active fly animations
    void updateFlyAnimations(float deltaTime) noexcept {
        for (auto& anim : flyAnimations) {
            if (anim.active) anim.update(deltaTime);
        }
    }
    
    // ✅ Called every frame to free completed animation slots
    // (Animations auto-deactivate in update(), so this is primarily for interface compatibility)
    void cleanupFinishedFlyAnimations() noexcept {
        // No-op: CoinFlyAnimation::update() sets active=false on completion.
        // Method retained for API compatibility with your existing client code.
    }
    
    // ✅ Spawn a new fly animation (returns false if pool exhausted)
    [[nodiscard]] bool spawnFlyAnimation(const glm::vec2& startPos, const glm::vec2& targetPos) noexcept {
        for (auto& anim : flyAnimations) {
            if (!anim.active) {
                anim.start(startPos, targetPos);
                return true;
            }
        }
        return false;  // Pool exhausted — client should handle gracefully
    }

    // === Queries (for client logic) ===
    [[nodiscard]] bool hasActiveCoins() const noexcept {
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].state == CoinState::Active) return true;
        return false;
    }
    
    [[nodiscard]] int getRenderableCount() const noexcept {
        int n = 0;
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].isRenderable()) ++n;
        return n;
    }
    
    [[nodiscard]] const std::array<Coin, MAX_COINS>& getCoins() const noexcept { return coins; }
    [[nodiscard]] int getActiveCount() const noexcept { return activeCount; }
    [[nodiscard]] int getActiveFlyCount() const noexcept {
        int n = 0;
        for (const auto& a : flyAnimations) if (a.active) ++n;
        return n;
    }

    // === Reset/Respawn ===
    void reset() noexcept {
        activeCount = 0;
        for (auto& c : coins) {
            c.state = CoinState::Dead;
            c.flyTriggered = false;
        }
        for (auto& a : flyAnimations) a.active = false;
        emptyTimer = 0.0f;
    }
    
    // ✅ Auto-respawn coins after all are collected + delay
    bool autoRespawnIfNeeded(CoinPattern pattern, int count, float deltaTime) {
        if (getRenderableCount() == 0) {
            emptyTimer += deltaTime;
            if (emptyTimer >= RESPAWN_DELAY) {
                initStars(pattern, count);
                return true;
            }
        } else {
            emptyTimer = 0.0f;
        }
        return false;
    }

private:
    // ✅ Pattern movement logic (timestep-independent via globalTime)
    void applyPattern(Coin& c, float globalTime) noexcept {
        constexpr float PI = 3.14159265358979323846f;
        
        switch (currentPattern) {
            case CoinPattern::SideToSide: {
                const float osc = std::sin(globalTime * SIDE_FREQUENCY * 2.0f * PI + c.phaseOffset);
                float targetX = c.basePosition.x + osc * SIDE_AMPLITUDE;
                c.position.x = std::clamp(targetX, 
                    -LANE_WIDTH * 0.5f + GUTTER_MARGIN, 
                     LANE_WIDTH * 0.5f - GUTTER_MARGIN);
                break;
            }
            case CoinPattern::WaveBop: {
                const float wave = std::sin(globalTime * HOVER_FREQUENCY + c.phaseOffset);
                c.position.y = 0.20f + wave * HOVER_AMPLITUDE;
                break;
            }
            case CoinPattern::Spiral: {
                const float angle = globalTime * SIDE_FREQUENCY * 2.0f * PI + c.phaseOffset;
                float offX = std::cos(angle) * SPIRAL_RADIUS;
                float offZ = std::sin(angle) * SPIRAL_RADIUS * 0.3f;
                c.position.x = std::clamp(c.basePosition.x + offX,
                    -LANE_WIDTH * 0.5f + GUTTER_MARGIN,
                     LANE_WIDTH * 0.5f - GUTTER_MARGIN);
                c.position.z = std::clamp(c.basePosition.z + offZ,
                    LANE_START_Z + 0.5f, LANE_END_Z - 0.3f);
                break;
            }
            default: // CoinPattern::Static
                c.position = c.basePosition;
                break;
        }
    }
};

// === Pattern selector (deterministic, hot-reload safe, no thread_local) ===
inline CoinPattern getNextCoinPattern() {
    static unsigned int idx = 0;
    const int max = static_cast<int>(CoinPattern::Count) - 1;
    return static_cast<CoinPattern>(idx++ % static_cast<unsigned int>(max));
}