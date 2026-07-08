#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <glm/glm.hpp>

enum class CountMastersOp : uint8_t
{
    ADD = 0,
    SUBTRACT = 1,
    MULTIPLY = 2,
    DIVIDE = 3,
};

struct CountMastersGateChoice
{
    CountMastersOp op = CountMastersOp::ADD;
    int value = 0;
};

struct CountMastersGateRow
{
    float z = 0.0f;
    CountMastersGateChoice left;
    CountMastersGateChoice right;
    bool resolved = false;
};

struct CountMastersEnemySquad
{
    float z = 0.0f;
    int count = 0;
    bool resolved = false;
};

enum class CountMastersPhase : uint8_t
{
    INACTIVE = 0,
    RUNNING = 1,
    WON = 2,
    LOST = 3,
};

struct CountMastersState
{
    static inline constexpr int MAX_UNITS = 64;
    static inline constexpr int GATE_COUNT = 4;
    static inline constexpr int ENEMY_SQUAD_COUNT = 4;
    static inline constexpr int PIN_COUNT = 10;
    static inline constexpr float LANE_HALF_WIDTH = 0.50f;
    static inline constexpr float START_Z = -1.10f;
    static inline constexpr float FINISH_Z = -15.15f;
    static inline constexpr float RUN_SPEED_MPS = 0.30625f;
    static inline constexpr float SIDE_FOLLOW_SPEED = 8.0f;

    CountMastersPhase phase = CountMastersPhase::INACTIVE;
    float runnerX = 0.0f;
    float targetX = 0.0f;
    float runnerZ = START_Z;
    float elapsed = 0.0f;
    int playerCount = 1;
    int pinsHit = 0;
    int rewardCoins = 0;
    std::array<CountMastersGateRow, GATE_COUNT> gates{};
    std::array<CountMastersEnemySquad, ENEMY_SQUAD_COUNT> enemies{};

    static inline int ApplyGateMath(int count, CountMastersGateChoice choice)
    {
        switch (choice.op)
        {
            case CountMastersOp::ADD:
                count += choice.value;
                break;
            case CountMastersOp::SUBTRACT:
                count -= choice.value;
                break;
            case CountMastersOp::MULTIPLY:
                count *= choice.value;
                break;
            case CountMastersOp::DIVIDE:
                count = (choice.value <= 0) ? count : count / choice.value;
                break;
        }
        return std::clamp(count, 0, MAX_UNITS);
    }

    static inline int ResolveFight(int players, int enemies)
    {
        return std::max(0, players - std::max(0, enemies));
    }

    static inline int ComputeRewardCoins(int survivors)
    {
        const int pins = std::min(PIN_COUNT, std::max(0, survivors));
        const int extra = std::max(0, survivors - PIN_COUNT);
        return pins * 4 + extra * 2;
    }

    static inline const char *OpSymbol(CountMastersOp op)
    {
        switch (op)
        {
            case CountMastersOp::ADD: return "+";
            case CountMastersOp::SUBTRACT: return "-";
            case CountMastersOp::MULTIPLY: return "x";
            case CountMastersOp::DIVIDE: return ":";
        }
        return "?";
    }

    static inline void FormatChoice(char *dst, int dstLen, CountMastersGateChoice choice)
    {
        if (!dst || dstLen <= 0)
            return;
        std::snprintf(dst, (size_t)dstLen, "%s%d", OpSymbol(choice.op), choice.value);
    }

    void initDefault()
    {
        phase = CountMastersPhase::RUNNING;
        runnerX = 0.0f;
        targetX = 0.0f;
        runnerZ = START_Z;
        elapsed = 0.0f;
        playerCount = 1;
        pinsHit = 0;
        rewardCoins = 0;

        gates = {{
            {-3.15f, {CountMastersOp::ADD, 5}, {CountMastersOp::MULTIPLY, 3}, false},
            {-6.10f, {CountMastersOp::MULTIPLY, 2}, {CountMastersOp::ADD, 12}, false},
            {-8.90f, {CountMastersOp::SUBTRACT, 4}, {CountMastersOp::MULTIPLY, 5}, false},
            {-11.70f, {CountMastersOp::DIVIDE, 2}, {CountMastersOp::ADD, 20}, false},
        }};
        enemies = {{
            {-4.35f, 2, false},
            {-7.25f, 9, false},
            {-10.05f, 13, false},
            {-13.10f, 18, false},
        }};
    }

    bool isActive() const
    {
        return phase == CountMastersPhase::RUNNING;
    }

    bool isDone() const
    {
        return phase == CountMastersPhase::WON || phase == CountMastersPhase::LOST;
    }

    void tick(float dt, float inputX)
    {
        if (phase != CountMastersPhase::RUNNING)
            return;

        dt = std::clamp(dt, 0.0f, 0.05f);
        elapsed += dt;
        targetX = std::clamp(inputX, -LANE_HALF_WIDTH, LANE_HALF_WIDTH);
        const float follow = std::clamp(dt * SIDE_FOLLOW_SPEED, 0.0f, 1.0f);
        runnerX += (targetX - runnerX) * follow;
        runnerZ -= RUN_SPEED_MPS * dt;

        for (CountMastersGateRow &gate : gates)
        {
            if (gate.resolved || runnerZ > gate.z)
                continue;
            gate.resolved = true;
            const CountMastersGateChoice choice = (runnerX < 0.0f) ? gate.left : gate.right;
            playerCount = ApplyGateMath(playerCount, choice);
            if (playerCount <= 0)
            {
                phase = CountMastersPhase::LOST;
                return;
            }
        }

        for (CountMastersEnemySquad &enemy : enemies)
        {
            if (enemy.resolved || runnerZ > enemy.z)
                continue;
            enemy.resolved = true;
            const int playersBeforeFight = playerCount;
            const int enemiesBeforeFight = enemy.count;
            playerCount = ResolveFight(playersBeforeFight, enemiesBeforeFight);
            enemy.count = std::max(0, enemiesBeforeFight - playersBeforeFight);
            if (playerCount <= 0)
            {
                phase = CountMastersPhase::LOST;
                return;
            }
        }

        if (runnerZ <= FINISH_Z)
        {
            pinsHit = std::min(PIN_COUNT, playerCount);
            rewardCoins = ComputeRewardCoins(playerCount);
            phase = (playerCount > 0) ? CountMastersPhase::WON : CountMastersPhase::LOST;
        }
    }
};
