#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef CHEST_RENDER_NO_GPU
#include "mesh.h"
#endif

namespace ChestRender
{
    // Bounds measured from assets/assman_out/chest.mesh after the Cube export.
    static constexpr glm::vec3 kMeshCenter(-0.16150159f, 1.06357682f, 0.33416384f);
    static constexpr float kWorldScale = 0.165f;
    static constexpr float kCollectibleWorldScale = 0.085f;
    static constexpr float kCollectiblePickupRadius = 0.35f;
    static constexpr float kSpinRadiansPerSecond = 1.70f;
    static constexpr float kSpawnChance = 1.0f;
    static constexpr float kSpawnDelayMinSeconds = 2.0f;
    static constexpr float kSpawnDelayMaxSeconds = 6.0f;
    static constexpr float kAvailableSeconds = 5.0f;
    static constexpr float kSpinInSeconds = 0.25f;
    static constexpr float kSpinOutSeconds = 0.25f;
    static constexpr float kCollectMoveSeconds = 0.52f;
    static constexpr float kAlignToFaceSeconds = 0.42f;
    static constexpr float kOpenSeconds = 0.46f;
    static constexpr float kRewardCloseSeconds = 0.70f;
    static constexpr float kRewardSpinOutSeconds = 0.42f;
    static constexpr float kCoinIntervalSeconds = 0.01f;

    enum class PrizeKind : uint8_t
    {
        Money25,
        Money50,
        RuneBoom,
        RuneBolt,
        RuneFreeze,
        RuneSkull,
        RuneGuardPins,
        RuneFootball,
    };

    struct SpawnChanceConfig
    {
        int level;
        int numerator;
        int denominator;
    };

    struct PrizeWeightConfig
    {
        int level;
        int weight;
        PrizeKind prize;
    };

    // Post-campaign random games deliberately use Level 12 (Ruined City) as
    // their chest reference. Their biome is random, but chest spawn and prize
    // rates stay fixed so the random scenery never changes progression odds.
    static constexpr int kPostCampaignReferenceLevel = 12;

    // Campaign chest appearance chance, rolled once when a player throw is
    // prepared. Levels omitted from this table (currently 1 and 2) cannot
    // spawn chests.
    // { level, numerator, denominator }
    // numerator: successful outcomes in the chance roll.
    // denominator: total equally likely outcomes in the chance roll.
    // Example: {3, 1, 15} means Level 3 has 1 successful outcome out of 15,
    // or a 1-in-15 (6.67%) chest chance per prepared player throw.
    static constexpr SpawnChanceConfig kSpawnChanceByLevel[] = {
        {3, 1, 15},  // Level 3: Red Desert / Malach
        {4, 1, 15},  // Level 4: Ice / Malach
        {5, 1, 14},  // Level 5: Suburbia / Malach
        {6, 1, 13},  // Level 6: Neon / Dog
        {7, 1, 12},  // Level 7: Desert / Dog
        {8, 1, 11},  // Level 8: Timber Zone / Dog
        {9, 1, 10},  // Level 9: Crystal Cavern / Beak
        {10, 1, 9},  // Level 10: Gas Factory / Beak
        {11, 1, 8},  // Level 11: Cemetery / Beak
        {12, 1, 7},  // Level 12: Ruined City / Cow
        {13, 1, 6},  // Level 13: Ashland / Cow
    };

    // Chest rewards by campaign level. Each level's positive weights add to
    // 100, so a weight is the percentage after that chest is collected. The
    // entries at levels 3 and 4 are intentionally cash-only. Boom is removed
    // and the remaining entries are reweighted when a player is not eligible
    // for another Boom rune; see AllowBoomPrizeForInventory(). The live
    // hoarding modifier is applied separately at collection time, so these
    // remain the base, designer-authored reward rates.
    // Keep one config element per line, grouped under its level heading, so
    // designers can review and edit the reward progression at a glance.
    // clang-format off
    static constexpr PrizeWeightConfig kPrizeWeightsByLevel[] = {
        // Level 3: Red Desert / Malach
        {3, 90, PrizeKind::Money25},
        {3, 10, PrizeKind::Money50},
        // Level 4: Ice / Malach
        {4, 80, PrizeKind::Money25},
        {4, 20, PrizeKind::Money50},
        // Level 5: Suburbia / Malach
        {5, 60, PrizeKind::Money25},
        {5, 30, PrizeKind::Money50},
        {5, 10, PrizeKind::RuneFootball},
        // Level 6: Neon / Dog
        {6, 45, PrizeKind::Money25},
        {6, 35, PrizeKind::Money50},
        {6, 15, PrizeKind::RuneBoom},
        {6, 5, PrizeKind::RuneBolt},
        {5, 10, PrizeKind::RuneFootball},
        // Level 7: Desert / Dog
        {7, 35, PrizeKind::Money25},
        {7, 35, PrizeKind::Money50},
        {7, 20, PrizeKind::RuneBoom},
        {7, 10, PrizeKind::RuneBolt},
        // Level 8: Timber Zone / Dog
        {8, 20, PrizeKind::Money25},
        {8, 25, PrizeKind::Money50},
        {8, 20, PrizeKind::RuneBoom},
        {8, 10, PrizeKind::RuneBolt},
        {8, 10, PrizeKind::RuneFreeze},
        {8, 10, PrizeKind::RuneSkull},
        {8, 5, PrizeKind::RuneGuardPins},
        // Level 9: Crystal Cavern / Beak
        {9, 15, PrizeKind::Money25},
        {9, 20, PrizeKind::Money50},
        {9, 20, PrizeKind::RuneBoom},
        {9, 15, PrizeKind::RuneBolt},
        {9, 10, PrizeKind::RuneFreeze},
        {9, 10, PrizeKind::RuneSkull},
        {9, 10, PrizeKind::RuneGuardPins},
        // Level 10: Gas Factory / Beak
        {10, 10, PrizeKind::Money25},
        {10, 15, PrizeKind::Money50},
        {10, 20, PrizeKind::RuneBoom},
        {10, 15, PrizeKind::RuneBolt},
        {10, 10, PrizeKind::RuneFreeze},
        {10, 15, PrizeKind::RuneSkull},
        {10, 15, PrizeKind::RuneGuardPins},
        // Level 11: Cemetery / Beak
        {11, 5, PrizeKind::Money25},
        {11, 10, PrizeKind::Money50},
        {11, 20, PrizeKind::RuneBoom},
        {11, 15, PrizeKind::RuneBolt},
        {11, 10, PrizeKind::RuneFreeze},
        {11, 20, PrizeKind::RuneSkull},
        {11, 10, PrizeKind::RuneGuardPins},
        // Level 12: Ruined City / Cow
        {12, 5, PrizeKind::Money25},
        {12, 10, PrizeKind::Money50},
        {12, 15, PrizeKind::RuneBoom},
        {12, 15, PrizeKind::RuneBolt},
        {12, 10, PrizeKind::RuneFreeze},
        {12, 15, PrizeKind::RuneSkull},
        {12, 15, PrizeKind::RuneGuardPins},
        {12, 15, PrizeKind::RuneFootball},
        // Level 13: Ashland / Cow
        {13, 0, PrizeKind::Money25},
        {13, 10, PrizeKind::Money50},
        {13, 15, PrizeKind::RuneBoom},
        {13, 15, PrizeKind::RuneBolt},
        {13, 10, PrizeKind::RuneFreeze},
        {13, 15, PrizeKind::RuneSkull},
        {13, 15, PrizeKind::RuneGuardPins},
        {13, 20, PrizeKind::RuneFootball},
    };
    // clang-format on

    enum class CollectiblePhase
    {
        Disabled,
        Waiting,
        Available,
        Expiring,
        CollectedMove,
        WaitingTap,
        Aligning,
        Opening,
        Payout,
        RewardClosing,
        RewardSpinOut,
    };

    inline bool IsCinematicActive(CollectiblePhase phase)
    {
        return phase == CollectiblePhase::CollectedMove ||
               phase == CollectiblePhase::WaitingTap;
    }

    inline bool IsRewardActive(CollectiblePhase phase)
    {
        return phase == CollectiblePhase::CollectedMove ||
               phase == CollectiblePhase::WaitingTap ||
               phase == CollectiblePhase::Aligning ||
               phase == CollectiblePhase::Opening ||
               phase == CollectiblePhase::Payout ||
               phase == CollectiblePhase::RewardClosing ||
               phase == CollectiblePhase::RewardSpinOut;
    }

    inline bool IsTextureActive(CollectiblePhase phase)
    {
        return IsCinematicActive(phase) ||
               phase == CollectiblePhase::Aligning ||
               phase == CollectiblePhase::Opening ||
               phase == CollectiblePhase::Payout ||
               phase == CollectiblePhase::RewardClosing ||
               phase == CollectiblePhase::RewardSpinOut;
    }

    inline uint8_t CinematicOverlayAlpha(CollectiblePhase phase)
    {
        return IsCinematicActive(phase) ? 172 : 0;
    }

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

    inline glm::mat4 ModelAt(const glm::vec3 &pos, float yawRadians, float scale)
    {
        return glm::translate(glm::mat4(1.0f), pos) *
               glm::rotate(glm::mat4(1.0f), yawRadians, glm::vec3(0.0f, 1.0f, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
               glm::translate(glm::mat4(1.0f), -kMeshCenter);
    }

#ifndef CHEST_RENDER_NO_GPU
    inline void ApplyEverythingAtlasParams(ShaderProgram &shader)
    {
        shader.updateTextureParamsInOneGo(
            glm::vec3(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            1.0f
        );
    }

    inline glm::vec3 WorldLightPos(float cameraZ)
    {
        return glm::vec3(3.0f, 3.0f, glm::clamp(cameraZ + 6.0f, -100.0f, -7.0f));
    }

    inline glm::vec3 ChestLightPos(const glm::vec3 &chestPos)
    {
        return chestPos + glm::vec3(-1.15f, 1.85f, -1.55f);
    }
#endif

    inline float PingPongOpenCloseTime(float seconds, float clipDuration)
    {
        return ChestState{seconds, 1.0f}.clipTime(clipDuration);
    }

    inline float Smooth01(float t)
    {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float ScaleForAvailable(float ageSeconds)
    {
        if (ageSeconds < kSpinInSeconds)
            return Smooth01(ageSeconds / kSpinInSeconds);
        const float outStart = kAvailableSeconds - kSpinOutSeconds;
        if (ageSeconds > outStart)
            return 1.0f - Smooth01((ageSeconds - outStart) / kSpinOutSeconds);
        return 1.0f;
    }

    inline float Deterministic01(int seed)
    {
        uint32_t x = (uint32_t)seed * 747796405u + 2891336453u;
        x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        x = (x >> 22u) ^ x;
        return (float)(x & 0x00FFFFFFu) / (float)0x01000000u;
    }

    inline SpawnChanceConfig SpawnChanceForLevel(int level)
    {
        for (const SpawnChanceConfig &cfg : kSpawnChanceByLevel)
        {
            if (cfg.level == level)
                return cfg;
        }
        return {level, 0, 1};
    }

    inline bool AllowBoomPrizeForInventory(int ownedBallCount, int carriedBoomRuneCount)
    {
        return ownedBallCount >= 2 && carriedBoomRuneCount <= 0;
    }

    inline bool PrizeIsRune(PrizeKind prize)
    {
        return prize == PrizeKind::RuneBoom || prize == PrizeKind::RuneBolt ||
               prize == PrizeKind::RuneFreeze || prize == PrizeKind::RuneSkull ||
               prize == PrizeKind::RuneGuardPins || prize == PrizeKind::RuneFootball;
    }

    // Four carried runes are free. Every rune beyond that halves the chance
    // that a chest awards any rune: five = 1/2, six = 1/4, seven = 1/8, etc.
    // The caller calculates this from the live inventory, so spending runes
    // immediately restores the configured level rates.
    inline float RunePrizeChanceMultiplierForCarriedRunes(int carriedRuneCount)
    {
        const int excess = glm::max(0, carriedRuneCount - 4);
        return std::ldexp(1.0f, -excess);
    }

    inline PrizeKind SelectPrizeForLevel(
        int level,
        float roll01,
        bool allowBoomPrize = true,
        float runePrizeChanceMultiplier = 1.0f)
    {
        int totalWeight = 0;
        for (const PrizeWeightConfig &cfg : kPrizeWeightsByLevel)
        {
            if (cfg.level == level && cfg.weight > 0 && (allowBoomPrize || cfg.prize != PrizeKind::RuneBoom))
                totalWeight += cfg.weight;
        }
        if (totalWeight <= 0)
            return PrizeKind::Money25;

        // Preserve the original configured distribution exactly while no
        // hoarding penalty applies.
        if (runePrizeChanceMultiplier >= 0.999999f)
        {
            int pick = glm::clamp((int)std::floor(glm::clamp(roll01, 0.0f, 0.999999f) * (float)totalWeight), 0, totalWeight - 1);
            for (const PrizeWeightConfig &cfg : kPrizeWeightsByLevel)
            {
                if (cfg.level != level || cfg.weight <= 0 || (!allowBoomPrize && cfg.prize == PrizeKind::RuneBoom))
                    continue;
                if (pick < cfg.weight)
                    return cfg.prize;
                pick -= cfg.weight;
            }
            return PrizeKind::Money25;
        }

        int runeWeight = 0;
        int moneyWeight = 0;
        for (const PrizeWeightConfig &cfg : kPrizeWeightsByLevel)
        {
            if (cfg.level != level || cfg.weight <= 0 ||
                (!allowBoomPrize && cfg.prize == PrizeKind::RuneBoom))
                continue;
            if (PrizeIsRune(cfg.prize))
                runeWeight += cfg.weight;
            else
                moneyWeight += cfg.weight;
        }

        const float baseRuneChance = (float)runeWeight / (float)totalWeight;
        const float runeChance = glm::clamp(
            baseRuneChance * glm::max(0.0f, runePrizeChanceMultiplier), 0.0f, 1.0f
        );
        const float clampedRoll = glm::clamp(roll01, 0.0f, 0.999999f);
        const bool chooseRune = runeWeight > 0 && clampedRoll < runeChance;
        const int categoryWeight = chooseRune ? runeWeight : moneyWeight;
        // Every configured level currently has a cash prize. Retain a safe
        // fallback for malformed or future tables rather than dividing by zero.
        if (categoryWeight <= 0)
            return PrizeKind::Money25;

        const float categoryRoll = chooseRune
            ? clampedRoll / glm::max(runeChance, 0.000001f)
            : (clampedRoll - runeChance) / glm::max(1.0f - runeChance, 0.000001f);
        int pick = glm::clamp(
            (int)std::floor(glm::clamp(categoryRoll, 0.0f, 0.999999f) * (float)categoryWeight),
            0, categoryWeight - 1
        );
        for (const PrizeWeightConfig &cfg : kPrizeWeightsByLevel)
        {
            if (cfg.level != level || cfg.weight <= 0 || (!allowBoomPrize && cfg.prize == PrizeKind::RuneBoom))
                continue;
            if (PrizeIsRune(cfg.prize) != chooseRune)
                continue;
            if (pick < cfg.weight)
                return cfg.prize;
            pick -= cfg.weight;
        }
        return PrizeKind::Money25;
    }
}
