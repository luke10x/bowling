#pragma once

#include "../../coins.h"

enum class MiniGameKind : uint8_t
{
    NONE = 0,
    COIN_RUSH = 1,
    COUNT_MASTERS = 2,
    CROWD_CONTROL = 3,
};

static inline MiniGameKind MiniGame_BonusForCampaignVictory(
    int levelNumber,
    bool defeatedMalachOrBetter,
    bool defeatedDogOrBetter,
    bool defeatedBeakOrBetter = false)
{
    if (levelNumber <= 1 || !defeatedMalachOrBetter)
        return MiniGameKind::NONE;
    if (levelNumber >= 13)
        return MiniGameKind::NONE;
    if (defeatedBeakOrBetter)
        return MiniGameKind::CROWD_CONTROL;
    return defeatedDogOrBetter ? MiniGameKind::COUNT_MASTERS : MiniGameKind::COIN_RUSH;
}

struct MiniGameCoinRush
{
    static inline constexpr int SLOT_COUNT = 7;
    static inline constexpr int MIN_COINS_PER_ROW = 2;
    static inline constexpr int MAX_COINS_PER_ROW = 3;
    static inline constexpr float ROW_SPACING_M = 0.68f;
    static inline constexpr float GRID_MARGIN_X_M = 0.06f;
    static inline constexpr float GRID_MARGIN_Z_M = 0.70f;
    static inline constexpr float COIN_Y_M = 0.20f;
    static inline constexpr float START_PHASE_OFFSET = 2.4f;

    static inline int ComputeColumnCount()
    {
        return SLOT_COUNT;
    }

    static inline int ComputeRowCount()
    {
        const float usableLength =
            (CoinLane::LANE_END_Z - GRID_MARGIN_Z_M) - (CoinLane::LANE_START_Z + GRID_MARGIN_Z_M);
        int rows = 1 + (int)(usableLength / ROW_SPACING_M);
        return std::max(1, rows);
    }

    static inline int CoinsInRow(int row)
    {
        return MIN_COINS_PER_ROW + ((row * 1103515245u + 12345u) >> 29u) % (MAX_COINS_PER_ROW - MIN_COINS_PER_ROW + 1);
    }

    static inline bool SlotHasCoin(int row, int slot)
    {
        if (row < 0 || slot < 0 || slot >= SLOT_COUNT)
            return false;
        int wanted = CoinsInRow(row);
        int found = 0;
        const uint32_t seed = (uint32_t)(row + 1) * 2654435761u;
        for (int step = 0; step < SLOT_COUNT && found < wanted; ++step)
        {
            const int candidate = (int)((seed + (uint32_t)step * 3u + (uint32_t)(row & 1)) % (uint32_t)SLOT_COUNT);
            bool duplicate = false;
            for (int prev = 0; prev < step; ++prev)
            {
                const int prevCandidate =
                    (int)((seed + (uint32_t)prev * 3u + (uint32_t)(row & 1)) % (uint32_t)SLOT_COUNT);
                if (prevCandidate == candidate)
                    duplicate = true;
            }
            if (duplicate)
                continue;
            if (candidate == slot)
                return true;
            ++found;
        }
        return false;
    }

    static inline int ComputeCoinCount()
    {
        int total = 0;
        const int rows = ComputeRowCount();
        for (int row = 0; row < rows; ++row)
            total += CoinsInRow(row);
        return total;
    }

    static inline void InitCoinGrid(CoinLane *lane)
    {
        if (!lane)
            return;

        const int columns = ComputeColumnCount();
        const int rows = ComputeRowCount();
        const float slotSpacing = (CoinLane::LANE_WIDTH - 2.0f * GRID_MARGIN_X_M) / (float)(columns - 1);
        const float fullWidth = slotSpacing * (float)(columns - 1);
        const float startX = -0.5f * fullWidth;
        const float startZ = CoinLane::LANE_START_Z + GRID_MARGIN_Z_M;

        lane->currentPattern = CoinPattern::SideToSide;
        lane->visualKind = CollectableVisualKind::Coin;
        lane->deployedGemCount = 0;
        lane->activeCount = std::min(CoinLane::MAX_COINS, ComputeCoinCount());
        lane->emptyTimer = 0.0f;

        int idx = 0;
        for (int row = 0; row < rows && idx < lane->activeCount; ++row)
        {
            for (int col = 0; col < columns && idx < lane->activeCount; ++col)
            {
                if (!SlotHasCoin(row, col))
                    continue;
                Coin &c = lane->coins[idx++];
                c.state = CoinState::Active;
                c.flyTriggered = false;
                c.visualKind = CollectableVisualKind::Coin;
                c.anchorIndex = -1;
                c.orbitXRadius = 0.0f;
                c.orbitZRadius = 0.0f;
                c.orbitSpeed = 0.0f;
                c.orbitPhase = 0.0f;
                c.orbitXSign = 1.0f;
                c.orbitZSign = 1.0f;
                c.rotation = 0.0f;
                c.scale = 1.0f;
                c.phaseOffset = START_PHASE_OFFSET + (float)row * 0.18f;
                c.position = glm::vec3(
                    startX + (float)col * slotSpacing,
                    COIN_Y_M,
                    startZ + (float)row * ROW_SPACING_M
                );
                c.basePosition = c.position;
                c.updateTransform();
            }
        }

        for (int i = lane->activeCount; i < CoinLane::MAX_COINS; ++i)
        {
            lane->coins[i].state = CoinState::Dead;
            lane->coins[i].flyTriggered = false;
            lane->coins[i].visualKind = CollectableVisualKind::Coin;
            lane->coins[i].anchorIndex = -1;
            lane->coins[i].orbitXRadius = 0.0f;
            lane->coins[i].orbitZRadius = 0.0f;
            lane->coins[i].orbitSpeed = 0.0f;
            lane->coins[i].orbitPhase = 0.0f;
            lane->coins[i].orbitXSign = 1.0f;
            lane->coins[i].orbitZSign = 1.0f;
        }

        for (auto &fly : lane->flyAnimations)
            fly.active = false;
    }
};
