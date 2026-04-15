#pragma once
#define GLM_ENABLE_EXPERIMENTAL 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <array>
#include <cmath>
#include <random>

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
    
    inline void updateTransform() noexcept {
        transform = glm::translate(glm::mat4(1.0f), position);
        transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(scale));
    }
    
    [[nodiscard]] inline bool isRenderable() const noexcept {
        // Must be alive AND visibly large enough (implosion complete if scale <= 0.05)
        return state != CoinState::Dead && scale > 0.05f;
    }
};

struct CoinLane {
    static constexpr int MAX_COINS = 10;
    static constexpr float LANE_START_Z = -18.288f + 3.0f;
    static constexpr float LANE_END_Z = 0.0f - 1.0f;
    static constexpr float LANE_WIDTH = 1.054f;
    static constexpr float GUTTER_MARGIN = 0.02f;
    static constexpr float BALL_RADIUS = 0.108f;
    static constexpr float COIN_HEIGHT = 0.20f;
    static constexpr float PICKUP_RADIUS_SQ = (BALL_RADIUS + 0.15f) * (BALL_RADIUS + 0.15f);
    static constexpr float IMPLODE_DURATION = 0.3f;
    
    std::array<Coin, MAX_COINS> coins{};
    int activeCount = 0;
    CoinPattern currentPattern = CoinPattern::Static;
    
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
                case CoinPattern::Static:
                default:
                    x = 0.0f;
                    break;
            }
            
            // Clamp to playable lane area
            x = (x < -LANE_WIDTH * 0.5f + GUTTER_MARGIN) ? -LANE_WIDTH * 0.5f + GUTTER_MARGIN : 
                (x > LANE_WIDTH * 0.5f - GUTTER_MARGIN) ? LANE_WIDTH * 0.5f - GUTTER_MARGIN : x;
            z = (z < LANE_START_Z + 0.5f) ? LANE_START_Z + 0.5f : 
                (z > LANE_END_Z - 0.3f) ? LANE_END_Z - 0.3f : z;
            
            coin.basePosition = glm::vec3(x, COIN_HEIGHT, z);
            coin.position = coin.basePosition;
            coin.phaseOffset = static_cast<float>(i) * 0.628f;
            coin.rotation = 0.0f;
            coin.scale = 1.0f;
            coin.state = CoinState::Active;
            coin.collected = false;
            coin.implosionProgress = 0.0f;
            coin.updateTransform();
        }
        
        for (int i = activeCount; i < MAX_COINS; ++i) {
            coins[i].state = CoinState::Dead;
        }
    }
    
    void updateStars(const glm::vec3& ballPos, float globalTime, float deltaTime) {
        for (int i = 0; i < activeCount; ++i) {
            Coin& coin = coins[i];
            if (coin.state == CoinState::Dead) continue;
            
            if (coin.state == CoinState::Active) {
                const glm::vec3 toBall = ballPos - coin.position;
                if (glm::dot(toBall, toBall) < PICKUP_RADIUS_SQ) {
                    coin.state = CoinState::Collected;
                    coin.collected = true;
                    coin.implosionProgress = 0.0f;
                }
            }
            
            if (coin.state == CoinState::Collected || coin.state == CoinState::Imploding) {
                coin.implosionProgress += deltaTime / IMPLODE_DURATION;
                if (coin.implosionProgress >= 1.0f) {
                    coin.state = CoinState::Dead;
                    continue;
                }
                // Manual smoothstep for constexpr compatibility
                const float easeT = (coin.implosionProgress < 0.0f) ? 0.0f : 
                                   (coin.implosionProgress > 1.0f) ? 1.0f : 
                                   coin.implosionProgress * coin.implosionProgress * (3.0f - 2.0f * coin.implosionProgress);
                coin.scale = 1.0f + easeT * (0.01f - 1.0f);
                coin.rotation += params.rotationSpeed * 2.5f * deltaTime;
                coin.updateTransform();
                continue;
            }
            
            if (coin.state == CoinState::Active) {
                applyPatternMovement(coin, globalTime);
                coin.rotation += params.rotationSpeed * deltaTime;
                coin.updateTransform();
            }
        }
    }
    
    [[nodiscard]] bool hasActiveCoins() const noexcept {
        for (int i = 0; i < activeCount; ++i)
            if (coins[i].state == CoinState::Active) return true;
        return false;
    }
    
    // [[nodiscard]] int getRenderableCount() const noexcept {
    //     int count = 0;
    //     for (const auto& coin : coins)
    //         if (coin.isRenderable()) ++count;
    //     return count;
    // }
    
[[nodiscard]] int getRenderableCount() const noexcept {
    int count = 0;
    for (int i = 0; i < activeCount; ++i) // Only check initialized coins
        if (coins[i].isRenderable()) ++count;
    return count;
}


    [[nodiscard]] const std::array<Coin, MAX_COINS>& getCoins() const noexcept { return coins; }
    [[nodiscard]] int getActiveCount() const noexcept { return activeCount; }
    
    void reset() noexcept {
        activeCount = 0;
        for (auto& coin : coins) coin.state = CoinState::Dead;
    }
    
    void applyPatternMovement(Coin& coin, float globalTime) noexcept {
        switch (currentPattern) {
            case CoinPattern::SideToSide: {
                const float oscillation = std::sin(
                    globalTime * params.sideFrequency * 2.0f * 3.14159265358979323846f 
                    + coin.phaseOffset
                );
                float targetX = coin.basePosition.x + oscillation * params.sideAmplitude;
                coin.position.x = (targetX < -LANE_WIDTH * 0.5f + GUTTER_MARGIN) ? -LANE_WIDTH * 0.5f + GUTTER_MARGIN : 
                                 (targetX > LANE_WIDTH * 0.5f - GUTTER_MARGIN) ? LANE_WIDTH * 0.5f - GUTTER_MARGIN : targetX;
                break;
            }
            case CoinPattern::WaveBop: {
                const float wave = std::sin(globalTime * params.waveSpeed + coin.phaseOffset);
                coin.position.y = COIN_HEIGHT + wave * params.hoverAmplitude;
                break;
            }
            case CoinPattern::Spiral: {
                const float angle = globalTime * params.sideFrequency * 2.0f * 3.14159265358979323846f 
                                  + coin.phaseOffset;
                float offsetX = std::cos(angle) * params.spiralRadius;
                float offsetZ = std::sin(angle) * params.spiralRadius * 0.3f;
                coin.position.x = (coin.basePosition.x + offsetX < -LANE_WIDTH * 0.5f + GUTTER_MARGIN) ? -LANE_WIDTH * 0.5f + GUTTER_MARGIN : 
                                 (coin.basePosition.x + offsetX > LANE_WIDTH * 0.5f - GUTTER_MARGIN) ? LANE_WIDTH * 0.5f - GUTTER_MARGIN : coin.basePosition.x + offsetX;
                float newZ = coin.basePosition.z + offsetZ;
                coin.position.z = (newZ < LANE_START_Z + 0.5f) ? LANE_START_Z + 0.5f : 
                                 (newZ > LANE_END_Z - 0.3f) ? LANE_END_Z - 0.3f : newZ;
                break;
            }
            case CoinPattern::Static:
            default:
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
                coin.updateTransform();
            }
        }
    }
        // Add near other variables
    float emptyTimer = 0.0f;
    static constexpr float RESPAWN_DELAY = 0.5f; // Seconds to wait before new pattern

    // Call this once per frame instead of manual checks
    bool autoRespawnIfNeeded(CoinPattern pattern, int count, float deltaTime) {
        if (getRenderableCount() == 0) {
            emptyTimer += deltaTime;
            if (emptyTimer >= RESPAWN_DELAY) {
                initStars(pattern, count);
                emptyTimer = 0.0f;
                return true; // Respawn triggered
            }
        } else {
            emptyTimer = 0.0f; // Reset if coins are still alive
        }
        return false;
    }
};

inline CoinPattern getRandomCoinPattern() {
    // Seeded once, reused across calls (fast & thread-safe in most game loops)
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(
        0, 
        static_cast<int>(CoinPattern::Count) - 1
    );
    return static_cast<CoinPattern>(dist(rng));
}