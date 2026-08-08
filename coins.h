#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm> // for std::clamp

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
enum class CollectableVisualKind : uint8_t {
    Coin = 0,
    Gem = 1,
    RuneBoom = 2,
    RuneBolt = 3,
    RuneFreeze = 4,
};

static inline bool CollectableVisualKind_IsRune(CollectableVisualKind kind) noexcept
{
    return kind == CollectableVisualKind::RuneBoom ||
           kind == CollectableVisualKind::RuneBolt ||
           kind == CollectableVisualKind::RuneFreeze;
}

struct CoinFlyAnimation {
    glm::vec2 startPos{};      // screen position where coin was collected
    glm::vec2 targetPos{};     // HUD destination
    CollectableVisualKind visualKind = CollectableVisualKind::Coin;
    float arcHeight = CoinFlyConfig::ARC_HEIGHT;
    float arcPhase = 0.0f;
    bool awardsPlayerBank = false;
    float elapsed = 0.0f;      // accumulated time since start
    float startDelay = 0.0f;   // seconds to wait before this fly becomes visible/moving
    bool playSfxOnStart = false;
    bool startSfxPending = false;
    bool active = false;       // is this animation slot in use?
    
    // Computed each update (for rendering)
    glm::vec2 currentPos{};
    float currentScale = 1.0f;
    float rotationY = 0.0f;

    static inline const float SPIN_SPEED = 18.0f; // radians per second

    void start(
        const glm::vec2& screenPos,
        const glm::vec2& target,
        CollectableVisualKind kind = CollectableVisualKind::Coin,
        bool awardsBank = false,
        float flyArcHeight = CoinFlyConfig::ARC_HEIGHT,
        float delaySeconds = 0.0f,
        bool playPickupSfxOnStart = false,
        float flyArcPhase = 0.0f
    ) {
        startPos = screenPos;
        targetPos = target;
        visualKind = kind;
        arcHeight = flyArcHeight;
        arcPhase = flyArcPhase;
        awardsPlayerBank = awardsBank;
        startDelay = std::max(0.0f, delaySeconds);
        playSfxOnStart = playPickupSfxOnStart;
        startSfxPending = playSfxOnStart;
        elapsed = -startDelay;
        active = true;
        currentPos = screenPos;
        currentScale = CoinFlyConfig::START_SCALE;
        rotationY = 0.0f;
    }

    // ✅ TIMESTEP-SAFE: No clamping, no frame-dependency
    // Works correctly even if deltaTime is 0.1s (10 FPS) or 0.5s (2 FPS)
    [[ nodiscard]] int updateOneFlyAnimation(float deltaTime, int *startedSfxCount = nullptr) {
        if (!active) return 0;
        
        elapsed += deltaTime;  // Accumulate real time
        if (elapsed < 0.0f)
            return false;
        if (startSfxPending)
        {
            if (startedSfxCount)
                *startedSfxCount += 1;
            startSfxPending = false;
        }
        const float duration = CoinFlyConfig::FLY_DURATION;
        
        // Normalized progress [0,1], clamped
        float t = elapsed / duration;
        if (t >= 1.0f) {
            // Animation complete: snap to target, deactivate
            currentPos = targetPos;
            currentScale = CoinFlyConfig::END_SCALE;
            active = false;  // Slot freed for reuse
            
            return true; // animation ended
        }

        // Smooth ease-in-out cubic (independent of frame rate)
        const float ease = (t < 0.5f) 
            ? 4.0f * t * t * t 
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;

        // Interpolate position with optional arc
        currentPos = glm::mix(startPos, targetPos, ease);
        if (arcHeight != 0.0f) {
            const float envelope = std::sin(t * 3.14159265f) * (1.0f - t);
            currentPos.y += envelope * arcHeight;
            currentPos.x += envelope * std::sin(t * 6.28318530f + arcPhase) * arcHeight * 0.36f;
        }

        // Interpolate scale
        currentScale = glm::mix(CoinFlyConfig::START_SCALE, CoinFlyConfig::END_SCALE, ease);
        
        // Continuous spin (timestep-independent: radians = speed * seconds)
        rotationY += SPIN_SPEED * deltaTime;

        return false; // not ended
    }

    [[nodiscard]] bool isComplete() const noexcept { return !active; }
    [[nodiscard]] bool isVisible() const noexcept { return active && elapsed >= 0.0f; }
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
    StaticDrift, // Like Static, but the gem drifts gently in lane-space
    SideSweep,   // SideToSide with extra Z wander
    WaveOrbit,   // WaveBop with a wider follow-on ring
    RibbonOrbit,  // Spiral-ish ribbon with more depth than width
    TwinOrbit,   // Two gems, orbiting coin clusters
    TripleOrbit, // Three gems, orbiting coin clusters
    Count
};

inline CoinPattern getNextCoinPattern();

inline float School_StrikeSwapDelayForZ(float z, float minZ, float maxZ, float maxDelay = 1.0f)
{
    if (!std::isfinite(z) || !std::isfinite(minZ) || !std::isfinite(maxZ) || !std::isfinite(maxDelay))
        return 0.0f;
    if (maxDelay <= 0.0f)
        return 0.0f;
    float range = maxZ - minZ;
    if (!std::isfinite(range) || std::abs(range) < 1e-6f)
        return 0.0f;
    float normalized = (z - minZ) / range;
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    return normalized * maxDelay;
}

// -----------------------------------------------------------------------------
// Coin — single collectible object
// -----------------------------------------------------------------------------
struct Coin {
    glm::vec3 position{};        // Current world position (updated by pattern)
    glm::vec3 basePosition{};    // Rest position for pattern calculations
    glm::mat4 transform{1.0f};   // Model matrix for rendering
    CollectableVisualKind visualKind = CollectableVisualKind::Coin;
    int anchorIndex = -1;        // Which gem this coin orbits around (-1 = self / anchor)
    float orbitXRadius = 0.0f;   // Orbit distance around the anchor on X
    float orbitZRadius = 0.0f;   // Orbit distance around the anchor on Z
    float orbitSpeed = 0.0f;     // Orbit angular speed
    float orbitPhase = 0.0f;     // Orbit phase offset
    float orbitXSign = 1.0f;     // Signed dispersion direction on X
    float orbitZSign = 1.0f;     // Signed dispersion direction on Z
    
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
    static inline const int MAX_COINS = 768;
    static inline const int MAX_FLY_ANIMATIONS = 768;
    
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
    static inline const float MIN_ORBIT_X = 0.50f;
    static inline const float MIN_ORBIT_Z = 3.00f;
    
    // Respawn timing
    static inline const float RESPAWN_DELAY = 0.5f;    // seconds before auto-respawn
    
    // === Data ===
    std::array<Coin, MAX_COINS> coins{};
    int activeCount = 0;
    CoinPattern currentPattern = CoinPattern::Static;
    CollectableVisualKind visualKind = CollectableVisualKind::Coin;
    int deployedGemCount = 0;
    
    // ✅ Public for your render loop (matches your existing code)
    std::array<CoinFlyAnimation, MAX_FLY_ANIMATIONS> flyAnimations{};
    
    float emptyTimer = 0.0f;

    // Add this to struct CoinLane (public section):
    void markFlyTriggered(int coinIndex) noexcept {
        if (coinIndex >= 0 && coinIndex < activeCount) {
            coins[coinIndex].flyTriggered = true;
        }
    }

    [[nodiscard]] int patternGemCount(CoinPattern pattern) const noexcept
    {
        switch (pattern)
        {
            case CoinPattern::StaticDrift:
            case CoinPattern::SideSweep:
                return 2;
            case CoinPattern::WaveOrbit:
            case CoinPattern::RibbonOrbit:
                return 3;
            case CoinPattern::TwinOrbit:
                return 2;
            case CoinPattern::TripleOrbit:
                return 3;
            default:
                return 1;
        }
    }

    [[nodiscard]] bool hasActiveGem() const noexcept
    {
        for (int i = 0; i < activeCount; ++i)
        {
            if (coins[i].visualKind == CollectableVisualKind::Gem && coins[i].state == CoinState::Active)
                return true;
        }
        return false;
    }

    [[nodiscard]] bool hasAnyGem() const noexcept
    {
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].visualKind == CollectableVisualKind::Gem)
                return true;
        return false;
    }

    // === Initialization ===
    void initStars(CoinPattern pattern, int count = MAX_COINS) {
        currentPattern = pattern;
        activeCount = std::clamp(count, 0, MAX_COINS);
        deployedGemCount = std::clamp(patternGemCount(pattern), 0, activeCount);
        visualKind = CollectableVisualKind::Gem;

        struct AnchorSlot
        {
            glm::vec3 basePosition{};
            glm::vec3 motion{};
        };

        std::array<AnchorSlot, 3> anchors{};
        auto setAnchor = [&](int idx, glm::vec3 basePosition, glm::vec3 motion)
        {
            if (idx >= 0 && idx < (int)anchors.size())
            {
                anchors[idx].basePosition = basePosition;
                anchors[idx].motion = motion;
            }
        };

        const float y = 0.20f;
        switch (pattern)
        {
            case CoinPattern::SideToSide:
                setAnchor(0, {0.0f, y, -8.2f}, {0.020f, 0.0f, 0.045f});
                break;
            case CoinPattern::WaveBop:
                setAnchor(0, {0.0f, y, -7.8f}, {0.010f, 0.020f, 0.040f});
                break;
            case CoinPattern::Spiral:
                setAnchor(0, {0.0f, y, -7.5f}, {0.015f, 0.012f, 0.050f});
                break;
            case CoinPattern::StaticDrift:
                setAnchor(0, {-0.40f, y, -10.8f}, {0.015f, 0.006f, 0.035f});
                setAnchor(1, {+0.40f, y, -5.2f}, {-0.015f, 0.006f, 0.035f});
                break;
            case CoinPattern::SideSweep:
                setAnchor(0, {-0.42f, y, -11.4f}, {0.018f, 0.008f, 0.040f});
                setAnchor(1, {+0.42f, y, -4.6f}, {-0.018f, 0.008f, 0.040f});
                break;
            case CoinPattern::WaveOrbit:
                setAnchor(0, {-0.44f, y, -12.0f}, {0.016f, 0.010f, 0.040f});
                setAnchor(1, {0.00f, y, -8.0f}, {0.000f, 0.012f, 0.045f});
                setAnchor(2, {+0.44f, y, -4.0f}, {-0.016f, 0.010f, 0.040f});
                break;
            case CoinPattern::RibbonOrbit:
                setAnchor(0, {-0.45f, y, -12.3f}, {0.014f, 0.010f, 0.045f});
                setAnchor(1, {0.00f, y, -7.8f}, {0.000f, 0.012f, 0.050f});
                setAnchor(2, {+0.45f, y, -3.3f}, {-0.014f, 0.010f, 0.045f});
                break;
            case CoinPattern::TwinOrbit:
                // Keep the two gems far enough apart that a single pickup won't catch both.
                setAnchor(0, {-0.42f, y, -12.0f}, {0.012f, 0.010f, 0.040f});
                setAnchor(1, {+0.42f, y, -4.8f}, {-0.012f, 0.010f, 0.040f});
                break;
            case CoinPattern::TripleOrbit:
                // Spread the trio across the lane so they feel like separate pickups.
                setAnchor(0, {-0.46f, y, -12.5f}, {0.010f, 0.010f, 0.045f});
                setAnchor(1, {0.00f, y, -7.8f}, {0.008f, 0.014f, 0.050f});
                setAnchor(2, {+0.46f, y, -3.1f}, {-0.010f, 0.010f, 0.045f});
                break;
            default:
                setAnchor(0, {0.0f, y, -8.0f}, {0.010f, 0.0f, 0.040f});
                break;
        }

        const int gemCount = std::max(1, deployedGemCount);
        for (int i = 0; i < activeCount; ++i)
        {
            Coin &c = coins[i];
            c.state = CoinState::Active;
            c.flyTriggered = false;
            c.rotation = 0.0f;
            c.scale = 1.0f;
            c.phaseOffset = 0.628f * static_cast<float>(i + 1);

            if (i < gemCount)
            {
                const int anchorIdx = (i < (int)anchors.size()) ? i : 0;
                c.visualKind = CollectableVisualKind::Gem;
                c.anchorIndex = -1;
                c.orbitXRadius = 0.0f;
                c.orbitZRadius = 0.0f;
                c.orbitSpeed = 0.0f;
                c.orbitPhase = 0.0f;
                c.orbitXSign = 1.0f;
                c.orbitZSign = 1.0f;
                c.basePosition = anchors[anchorIdx].basePosition;
                c.phaseOffset = 1.047f * static_cast<float>(anchorIdx);
                c.position = c.basePosition;
            }
            else
            {
                const int anchorIdx = (gemCount > 0) ? ((i - gemCount) % gemCount) : 0;
                const int ring = (gemCount > 0) ? ((i - gemCount) / gemCount) : 0;
                c.visualKind = CollectableVisualKind::Coin;
                c.anchorIndex = anchorIdx;
                c.orbitXRadius = MIN_ORBIT_X + 0.08f * static_cast<float>(ring);
                c.orbitZRadius = MIN_ORBIT_Z + 0.55f * static_cast<float>(ring);
                c.orbitSpeed = 1.4f + 0.16f * static_cast<float>(ring) + 0.08f * static_cast<float>(anchorIdx);
                c.orbitPhase = 0.85f * static_cast<float>(i);
                c.orbitXSign = ((i - gemCount) % 2 == 0) ? -1.0f : 1.0f;
                c.orbitZSign = (((i - gemCount) / 2) % 2 == 0) ? -1.0f : 1.0f;
                c.basePosition = anchors[anchorIdx].basePosition;
                c.position = c.basePosition;
            }

            c.updateTransform();
        }

        for (int i = activeCount; i < MAX_COINS; ++i)
        {
            coins[i].state = CoinState::Dead;
            coins[i].flyTriggered = false;
            coins[i].visualKind = CollectableVisualKind::Coin;
            coins[i].anchorIndex = -1;
            coins[i].orbitXRadius = 0.0f;
            coins[i].orbitZRadius = 0.0f;
            coins[i].orbitSpeed = 0.0f;
            coins[i].orbitPhase = 0.0f;
            coins[i].orbitXSign = 1.0f;
            coins[i].orbitZSign = 1.0f;
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
    [[nodiscard]] int updateFlyAnimations(float deltaTime, int *startedSfxCount = nullptr) noexcept {
        int earnings = 0;
        for (auto& anim : flyAnimations) {
            if (anim.active) {
                if (anim.updateOneFlyAnimation(deltaTime, startedSfxCount)) {
                    if (anim.awardsPlayerBank)
                        earnings += 1;
                };
            }
        }
        return earnings;
    }
    
    // ✅ Called every frame to free completed animation slots
    // (Animations auto-deactivate in update(), so this is primarily for interface compatibility)
    void cleanupFinishedFlyAnimations() noexcept {
        // No-op: CoinFlyAnimation::update() sets active=false on completion.
        // Method retained for API compatibility with your existing client code.
    }
    
    // ✅ Spawn a new fly animation (returns false if pool exhausted)
    [[nodiscard]] bool spawnFlyAnimation(
        const glm::vec2& startPos,
        const glm::vec2& targetPos,
        CollectableVisualKind kind = CollectableVisualKind::Coin,
        bool awardsPlayerBank = false,
        float arcHeight = CoinFlyConfig::ARC_HEIGHT,
        float startDelay = 0.0f,
        bool playPickupSfxOnStart = false,
        float arcPhase = 0.0f
    ) noexcept {
        for (auto& anim : flyAnimations) {
            if (!anim.active) {
                anim.start(
                    startPos,
                    targetPos,
                    kind,
                    awardsPlayerBank,
                    arcHeight,
                    startDelay,
                    playPickupSfxOnStart,
                    arcPhase
                );
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
    [[nodiscard]]int resetAllAnimations() noexcept {
        activeCount = 0;
        for (auto& c : coins) {
            c.state = CoinState::Dead;
            c.flyTriggered = false;
        }
        int earnings = 0;
        for (auto& a : flyAnimations) {
            a.active = false;
            earnings += 1;
        }
        emptyTimer = 0.0f;
        return earnings;
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

    bool redistributeIfAllGemsCollected() {
        if (activeCount <= 0)
            return false;
        if (!hasAnyGem() || hasActiveGem())
            return false;
        initStars(getNextCoinPattern(), activeCount);
        return true;
    }

private:
    // ✅ Pattern movement logic (timestep-independent via globalTime)
    void applyPattern(Coin& c, float globalTime) noexcept {
        constexpr float PI = 3.14159265358979323846f;

        auto clampLane = [&](glm::vec3 p) {
            p.x = std::clamp(p.x, -LANE_WIDTH * 0.5f + GUTTER_MARGIN, LANE_WIDTH * 0.5f - GUTTER_MARGIN);
            p.z = std::clamp(p.z, LANE_START_Z + 0.5f, LANE_END_Z - 0.3f);
            return p;
        };

        auto applyGemMotion = [&](Coin &gem)
        {
            float t = globalTime + gem.phaseOffset;
            glm::vec3 pos = gem.basePosition;
            // Keep the anchor itself moving across the lane so the pickup path
            // shifts from one side to the other instead of feeling parked.
            const float laneHalf = LANE_WIDTH * 0.5f - GUTTER_MARGIN - 0.02f;
            const float sweep = 0.22f + 0.05f * std::sin(t * 0.19f + gem.phaseOffset);
            const float xSweep = std::sin(t * 0.62f + gem.phaseOffset) * sweep;
            const float xDrift = std::sin(t * 1.35f + 1.1f + gem.phaseOffset) * 0.045f;
            pos.x += xSweep + xDrift;
            pos.z += std::cos(t * 0.35f + 0.7f + gem.phaseOffset) * 0.028f;
            // Multi-gem patterns should read as a loose counterclockwise orbit
            // with enough spacing that each gem stays distinct.
            const bool multiGemPattern =
                currentPattern == CoinPattern::StaticDrift ||
                currentPattern == CoinPattern::SideSweep ||
                currentPattern == CoinPattern::WaveOrbit ||
                currentPattern == CoinPattern::RibbonOrbit ||
                currentPattern == CoinPattern::TwinOrbit ||
                currentPattern == CoinPattern::TripleOrbit;
            const float orbitSpeed = multiGemPattern ? 0.42f : 0.72f;
            const float orbitRadiusX = multiGemPattern ? 0.10f : 0.12f;
            const float orbitRadiusZ = multiGemPattern ? 0.08f : 0.09f;
            const float orbitAngle = -(globalTime * orbitSpeed + gem.phaseOffset);
            pos.x += std::cos(orbitAngle) * orbitRadiusX;
            pos.z += std::sin(orbitAngle) * orbitRadiusZ;
            switch (currentPattern)
            {
                case CoinPattern::SideToSide:
                    pos.x += std::sin(t * 1.05f) * 0.08f;
                    pos.z += std::cos(t * 0.90f) * 0.030f;
                    pos.y += std::cos(t * 2.0f) * 0.008f;
                    break;
                case CoinPattern::WaveBop:
                    pos.y += std::sin(t * 2.2f) * 0.020f;
                    pos.x += std::sin(t * 0.75f) * 0.035f;
                    pos.z += std::cos(t * 0.95f) * 0.022f;
                    break;
                case CoinPattern::Spiral:
                    pos.x += std::cos(t * 1.35f) * 0.045f;
                    pos.z += std::sin(t * 1.35f) * 0.032f;
                    pos.y += std::sin(t * 2.1f) * 0.010f;
                    break;
                case CoinPattern::StaticDrift:
                    pos.x += std::sin(t * 0.65f) * 0.06f;
                    pos.z += std::cos(t * 1.05f) * 0.030f;
                    pos.y += std::sin(t * 1.2f) * 0.006f;
                    break;
                case CoinPattern::SideSweep:
                    pos.x += std::sin(t * 1.0f) * 0.12f;
                    pos.z += std::cos(t * 1.1f) * 0.042f;
                    pos.y += std::sin(t * 1.8f) * 0.009f;
                    break;
                case CoinPattern::WaveOrbit:
                    pos.x += std::sin(t * 0.85f) * 0.07f;
                    pos.z += std::cos(t * 1.25f) * 0.050f;
                    pos.y += std::sin(t * 2.3f) * 0.012f;
                    break;
                case CoinPattern::RibbonOrbit:
                    pos.x += std::cos(t * 1.05f) * 0.05f;
                    pos.z += std::sin(t * 1.45f) * 0.058f;
                    pos.y += std::cos(t * 1.95f) * 0.010f;
                    break;
                case CoinPattern::TwinOrbit:
                case CoinPattern::TripleOrbit:
                    pos.x += std::sin(t * 0.95f) * 0.04f;
                    pos.z += std::cos(t * 1.2f) * 0.030f;
                    pos.y += std::cos(t * 1.7f) * 0.010f;
                    break;
                default:
                    pos.x += std::sin(t * 0.55f) * 0.04f;
                    break;
            }
            pos.x = std::clamp(pos.x, -laneHalf, laneHalf);
            gem.position = clampLane(pos);
        };

        if (c.visualKind == CollectableVisualKind::Gem || c.anchorIndex < 0 || c.anchorIndex >= activeCount)
        {
            applyGemMotion(c);
            return;
        }

        const Coin &anchor = coins[c.anchorIndex];
        glm::vec3 pos = anchor.position;
        const float angle = globalTime * c.orbitSpeed + c.orbitPhase;
        pos.x += std::cos(angle) * c.orbitXRadius;
        pos.z += std::sin(angle) * c.orbitZRadius;
        pos.y += std::sin(angle * 1.8f + c.phaseOffset) * 0.026f;

        switch (currentPattern)
        {
            case CoinPattern::SideToSide:
                pos.x += std::cos(angle * 1.2f) * 0.12f;
                pos.z += std::sin(angle * 1.2f) * 0.06f;
                break;
            case CoinPattern::WaveBop:
                pos.y += std::cos(angle * 1.4f) * 0.012f;
                break;
            case CoinPattern::Spiral:
                pos.x += std::cos(angle * 1.1f) * 0.08f;
                pos.z += std::sin(angle * 1.1f) * 0.08f;
                break;
            case CoinPattern::StaticDrift:
                pos.x += std::cos(angle * 0.8f) * 0.05f;
                pos.z += std::sin(angle * 1.0f) * 0.05f;
                break;
            case CoinPattern::SideSweep:
                pos.x += std::cos(angle * 1.3f) * 0.15f;
                pos.z += std::sin(angle * 1.3f) * 0.05f;
                break;
            case CoinPattern::WaveOrbit:
                pos.x += std::cos(angle * 1.6f) * 0.10f;
                pos.z += std::sin(angle * 1.6f) * 0.10f;
                break;
            case CoinPattern::RibbonOrbit:
                pos.x += std::cos(angle * 1.9f) * 0.08f;
                pos.z += std::sin(angle * 1.9f) * 0.12f;
                break;
            case CoinPattern::TwinOrbit:
            case CoinPattern::TripleOrbit:
                pos.x += std::cos(angle * 1.5f) * 0.10f;
                pos.z += std::sin(angle * 1.5f) * 0.10f;
                pos.y += std::sin(angle * 1.5f) * 0.014f;
                break;
            default:
                break;
        }

        c.position = clampLane(pos);
    }
};

// === Pattern selector (deterministic, hot-reload safe, no thread_local) ===
inline CoinPattern getNextCoinPattern() {
    static const std::array<CoinPattern, 10> sequence = {
        CoinPattern::Static,
        CoinPattern::SideToSide,
        CoinPattern::StaticDrift,
        CoinPattern::WaveBop,
        CoinPattern::SideSweep,
        CoinPattern::Spiral,
        CoinPattern::WaveOrbit,
        CoinPattern::TwinOrbit,
        CoinPattern::RibbonOrbit,
        CoinPattern::TripleOrbit,
    };
    static unsigned int idx = 0;
    return sequence[idx++ % sequence.size()];
}
