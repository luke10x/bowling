#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <algorithm> // for std::clamp

// -----------------------------------------------------------------------------
// CoinFlyAnimation — tracks a single coin flying from 3D world to 2D HUD
// All tunable params are in CoinFlyConfig below (hot-reload friendly)
// -----------------------------------------------------------------------------

struct CoinFlyConfig {
    // Changed: static constexpr → static inline const (C++17) for hot reload
    static inline const float FLY_DURATION = 0.8f;
    static inline const float ARC_HEIGHT = 0.0f;
    static inline const float START_SCALE = 1.0f;
    static inline const float END_SCALE = 0.8f;
    static inline const float PIXEL_SIZE = 40.0f;
    static inline const float TARGET_X = 40.0f;
    static inline const float TARGET_Y = 40.0f;
};

struct CoinFlyAnimation {
    glm::vec2 startPos;
    glm::vec2 targetPos;
    float elapsed = 0.0f;
    bool active = false;
    glm::vec2 currentPos;
    float currentScale;
    float rotationY = 0.0f;

    static inline const float SPIN_SPEED = 8.0f; // hot-reload safe

    void start(const glm::vec2& screenPos, const glm::vec2& target) {
        startPos = screenPos; targetPos = target;
        elapsed = 0.0f; active = true;
        currentPos = screenPos; currentScale = CoinFlyConfig::START_SCALE;
        rotationY = 0.0f;
    }

    void update(float deltaTime) {
        if (!active) return;
        elapsed += deltaTime;
        float t = elapsed / CoinFlyConfig::FLY_DURATION;
        if (t >= 1.0f) { t = 1.0f; active = false; return; }

        float ease = (t < 0.5f) ? 4.0f*t*t*t : 1.0f - std::pow(-2.0f*t+2.0f, 3.0f)*0.5f;
        currentPos = glm::mix(startPos, targetPos, ease);
        currentPos.y -= std::sin(t * 3.14159265f) * CoinFlyConfig::ARC_HEIGHT;
        currentScale = glm::mix(CoinFlyConfig::START_SCALE, CoinFlyConfig::END_SCALE, ease);
        rotationY += SPIN_SPEED * deltaTime;
    }

    [[nodiscard]] bool isComplete() const { return !active; }
};

enum class CoinState : uint8_t {
    Active,
    Collected,
    Imploding,
    Dead
};

enum class CoinPattern : uint8_t {
    Static,
    SideToSide,
    WaveBop,
    Spiral,
    Count
};

struct Coin {
    glm::vec3 position = glm::vec3(0.0f);
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec3 basePosition = glm::vec3(0.0f);
    
    float rotation = 0.0f;
    float scale = 1.0f;
    float phaseOffset = 0.0f;
    
    CoinState state = CoinState::Dead;
    float implosionProgress = 0.0f;
    bool collected = false;
    bool flyTriggered = false; // Prevent duplicate fly-animation spawns
    
    void updateTransform() noexcept {
        transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(scale));
    }
    
    [[nodiscard]] bool isRenderable() const noexcept {
        return state != CoinState::Dead && scale > 0.05f;
    }
};

struct CoinLane {
    // Config values: static inline const for hot-reload compatibility
    static inline const int MAX_COINS = 10;
    static inline const int MAX_FLY_ANIMATIONS = 8; // Max concurrent flying coins
    static inline const float LANE_START_Z = -18.288f + 3.0f;
    static inline const float LANE_END_Z = 0.0f - 1.0f;
    static inline const float LANE_WIDTH = 1.054f;
    static inline const float GUTTER_MARGIN = 0.02f;
    static inline const float BALL_RADIUS = 0.108f;
    static inline const float COIN_HEIGHT = 0.20f;
    static inline const float PICKUP_RADIUS_SQ = (BALL_RADIUS + 0.15f) * (BALL_RADIUS + 0.15f);
    static inline const float IMPLODE_DURATION = 0.3f;
    
    std::array<Coin, MAX_COINS> coins{};
    int activeCount = 0;
    CoinPattern currentPattern = CoinPattern::Static;

    // Multiple concurrent fly-to-HUD animations
    std::array<CoinFlyAnimation, MAX_FLY_ANIMATIONS> flyAnimations{};

    struct PatternParams {
        float sideAmplitude = 0.70f;
        float sideFrequency = 0.5f;
        float waveSpeed = 3.5f;
        float hoverAmplitude = 0.03f;
        float rotationSpeed = 2.5f;
        float spiralRadius = 0.12f;
        float maxLateralOffset = LANE_WIDTH * 0.4f;
        float spinAxisTilt = 0.08f;
    } params;

    // Initialize coins in lane with specified pattern
    void initStars(CoinPattern pattern, int count = MAX_COINS) {
        currentPattern = pattern;
        activeCount = (count < 0) ? 0 : (count > MAX_COINS) ? MAX_COINS : count;
        
        for (int i = 0; i < activeCount; ++i) {
            Coin& coin = coins[i];
            const float t = (activeCount > 1) 
                ? static_cast<float>(i) / static_cast<float>(activeCount - 1) 
                : 0.5f;
            float z = LANE_START_Z + t * (LANE_END_Z - LANE_START_Z);
            float x = 0.0f;
            
            switch (pattern) {
                case CoinPattern::SideToSide:
                    x = ((i % 2 == 0) ? -1.0f : 1.0f) * LANE_WIDTH * 0.3f;
                    break;
                case CoinPattern::WaveBop:
                case CoinPattern::Spiral:
                    x = std::sin(static_cast<float>(i) * 1.3f) * LANE_WIDTH * 0.2f;
                    break;
                default: // Static
                    x = 0.0f;
                    break;
            }
            
            // Clamp to playable lane area
            x = std::clamp(x, -LANE_WIDTH * 0.5f + GUTTER_MARGIN, LANE_WIDTH * 0.5f - GUTTER_MARGIN);
            z = std::clamp(z, LANE_START_Z + 0.5f, LANE_END_Z - 0.3f);
            
            coin.basePosition = glm::vec3(x, COIN_HEIGHT, z);
            coin.position = coin.basePosition;
            coin.phaseOffset = static_cast<float>(i) * 0.628f; // ~2π/10
            coin.rotation = 0.0f;
            coin.scale = 1.0f;
            coin.state = CoinState::Active;
            coin.collected = false;
            coin.flyTriggered = false;
            coin.implosionProgress = 0.0f;
            coin.updateTransform();
        }
        
        for (int i = activeCount; i < MAX_COINS; ++i) {
            coins[i].state = CoinState::Dead;
        }
    }
    
    // Update coin logic: pickup detection, implosion, pattern movement
    void updateStars(const glm::vec3& ballPos, float globalTime, float deltaTime) {
        for (int i = 0; i < activeCount; ++i) {
            Coin& coin = coins[i];
            if (coin.state == CoinState::Dead) continue;
            
            // Pickup detection
            if (coin.state == CoinState::Active) {
                const glm::vec3 toBall = ballPos - coin.position;
                if (glm::dot(toBall, toBall) < PICKUP_RADIUS_SQ) {
                    coin.state = CoinState::Collected;
                    coin.collected = true;
                    coin.flyTriggered = false; // Allow fly animation to trigger
                    coin.implosionProgress = 0.0f;
                }
            }
            
            // Implosion/Collection animation
            if (coin.state == CoinState::Collected || coin.state == CoinState::Imploding) {
                coin.implosionProgress += deltaTime / IMPLODE_DURATION;
                if (coin.implosionProgress >= 1.0f) {
                    coin.state = CoinState::Dead;
                    continue;
                }
                const float easeT = std::clamp(coin.implosionProgress, 0.0f, 1.0f);
                const float smooth = easeT * easeT * (3.0f - 2.0f * easeT);
                coin.scale = 1.0f + smooth * (0.01f - 1.0f);
                coin.rotation += params.rotationSpeed * 2.5f * deltaTime;
                coin.updateTransform();
                continue;
            }
            
            // Active pattern movement
            if (coin.state == CoinState::Active) {
                applyPatternMovement(coin, globalTime);
                coin.rotation += params.rotationSpeed * deltaTime;
                coin.updateTransform();
            }
        }
    }
    
    // Update all flying coin animations
    void updateFlyAnimations(float deltaTime) noexcept {
        for (auto& anim : flyAnimations) {
            if (anim.active) anim.update(deltaTime);
        }
    }
    
    // Cleanup finished animations to free slots
    void cleanupFinishedFlyAnimations() noexcept {
        for (auto& anim : flyAnimations) {
            if (anim.active && anim.isComplete()) {
                anim.active = false;
            }
        }
    }
    
    // Spawn a new fly animation (returns false if pool exhausted)
    [[nodiscard]] bool spawnFlyAnimation(const glm::vec2& startPos, const glm::vec2& targetPos) noexcept {
        for (auto& anim : flyAnimations) {
            if (!anim.active) {
                anim.start(startPos, targetPos);
                return true;
            }
        }
        return false; // Pool exhausted
    }
    
    [[nodiscard]] bool hasActiveCoins() const noexcept {
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].state == CoinState::Active) return true;
        return false;
    }
    
    [[nodiscard]] int getRenderableCount() const noexcept {
        int count = 0;
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].isRenderable()) ++count;
        return count;
    }

    [[nodiscard]] const std::array<Coin, MAX_COINS>& getCoins() const noexcept { return coins; }
    [[nodiscard]] int getActiveCount() const noexcept { return activeCount; }
    [[nodiscard]] int getActiveFlyCount() const noexcept {
        int count = 0;
        for (const auto& anim : flyAnimations) if (anim.active) ++count;
        return count;
    }
    
    void reset() noexcept {
        activeCount = 0;
        for (auto& coin : coins) {
            coin.state = CoinState::Dead;
            coin.flyTriggered = false;
        }
        for (auto& anim : flyAnimations) anim.active = false;
    }
    
    void applyPatternMovement(Coin& coin, float globalTime) noexcept {
        constexpr float PI = 3.14159265358979323846f;
        switch (currentPattern) {
            case CoinPattern::SideToSide: {
                const float oscillation = std::sin(globalTime * params.sideFrequency * 2.0f * PI + coin.phaseOffset);
                float targetX = coin.basePosition.x + oscillation * params.sideAmplitude;
                coin.position.x = std::clamp(targetX, -LANE_WIDTH * 0.5f + GUTTER_MARGIN, LANE_WIDTH * 0.5f - GUTTER_MARGIN);
                break;
            }
            case CoinPattern::WaveBop: {
                const float wave = std::sin(globalTime * params.waveSpeed + coin.phaseOffset);
                coin.position.y = COIN_HEIGHT + wave * params.hoverAmplitude;
                break;
            }
            case CoinPattern::Spiral: {
                const float angle = globalTime * params.sideFrequency * 2.0f * PI + coin.phaseOffset;
                float offsetX = std::cos(angle) * params.spiralRadius;
                float offsetZ = std::sin(angle) * params.spiralRadius * 0.3f;
                coin.position.x = std::clamp(coin.basePosition.x + offsetX, -LANE_WIDTH * 0.5f + GUTTER_MARGIN, LANE_WIDTH * 0.5f - GUTTER_MARGIN);
                coin.position.z = std::clamp(coin.basePosition.z + offsetZ, LANE_START_Z + 0.5f, LANE_END_Z - 0.3f);
                break;
            }
            default: // Static
                coin.position = coin.basePosition;
                break;
        }
        
        if (params.spinAxisTilt > 0.0f && coin.position.z < -0.5f) {
            float tiltAngle = std::atan2(coin.position.z, -coin.position.x) * params.spinAxisTilt;
            coin.transform = glm::rotate(coin.transform, tiltAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        }
    }
    
    void activateAllCoins() noexcept {
        for (int i = 0; i < activeCount; ++i) {
            Coin& coin = coins[i];
            if (coin.state != CoinState::Active) {
                coin.state = CoinState::Active;
                coin.implosionProgress = 0.0f;
                coin.scale = 1.0f;
                coin.collected = false;
                coin.flyTriggered = false;
                coin.updateTransform();
            }
        }
    }
    
    float emptyTimer = 0.0f;
    static inline const float RESPAWN_DELAY = 0.5f;

    bool autoRespawnIfNeeded(CoinPattern pattern, int count, float deltaTime) {
        if (getRenderableCount() == 0) {
            emptyTimer += deltaTime;
            if (emptyTimer >= RESPAWN_DELAY) {
                initStars(pattern, count);
                emptyTimer = 0.0f;
                return true;
            }
        } else {
            emptyTimer = 0.0f;
        }
        return false;
    }
};

// Deterministic pattern selector (hot-reload safe, no thread_local)
CoinPattern getNextCoinPattern() {
    static unsigned int patternIndex = 0;
    const int maxPattern = static_cast<int>(CoinPattern::Count) - 1;
    const int result = static_cast<int>(patternIndex % static_cast<unsigned int>(maxPattern));
    ++patternIndex;
    return static_cast<CoinPattern>(result);
}