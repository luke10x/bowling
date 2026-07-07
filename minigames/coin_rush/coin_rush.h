#pragma once

#include "../../coins.h"

enum class MiniGameKind : uint8_t
{
    NONE = 0,
    COIN_RUSH = 1,
};

struct MiniGameCoinRush
{
    static inline constexpr float GRID_SPACING_M = 0.15f;
    static inline constexpr float GRID_MARGIN_X_M = 0.06f;
    static inline constexpr float GRID_MARGIN_Z_M = 0.45f;
    static inline constexpr float COIN_Y_M = 0.20f;

    static inline int ComputeColumnCount()
    {
        const float usableWidth = CoinLane::LANE_WIDTH - 2.0f * GRID_MARGIN_X_M;
        int columns = 1 + (int)(usableWidth / GRID_SPACING_M);
        return std::max(1, columns);
    }

    static inline int ComputeRowCount()
    {
        const float usableLength =
            (CoinLane::LANE_END_Z - GRID_MARGIN_Z_M) - (CoinLane::LANE_START_Z + GRID_MARGIN_Z_M);
        int rows = 1 + (int)(usableLength / GRID_SPACING_M);
        return std::max(1, rows);
    }

    static inline int ComputeCoinCount()
    {
        return ComputeColumnCount() * ComputeRowCount();
    }

    static inline void InitCoinGrid(CoinLane *lane)
    {
        if (!lane)
            return;

        const int columns = ComputeColumnCount();
        const int rows = ComputeRowCount();
        const float fullWidth = GRID_SPACING_M * (float)(columns - 1);
        const float startX = -0.5f * fullWidth;
        const float startZ = CoinLane::LANE_START_Z + GRID_MARGIN_Z_M;

        lane->currentPattern = CoinPattern::Static;
        lane->visualKind = CollectableVisualKind::Coin;
        lane->deployedGemCount = 0;
        lane->activeCount = std::min(CoinLane::MAX_COINS, columns * rows);
        lane->emptyTimer = 0.0f;

        int idx = 0;
        for (int row = 0; row < rows && idx < lane->activeCount; ++row)
        {
            for (int col = 0; col < columns && idx < lane->activeCount; ++col)
            {
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
                c.phaseOffset = 0.0f;
                c.position = glm::vec3(
                    startX + (float)col * GRID_SPACING_M,
                    COIN_Y_M,
                    startZ + (float)row * GRID_SPACING_M
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
