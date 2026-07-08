#pragma once

#include <algorithm>
#include <array>
#include <cmath>
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
    int chosenSide = 0; // -1 left, +1 right, 0 unresolved
};

struct CountMastersEnemySquad
{
    float z = 0.0f;
    int count = 0;
    bool resolved = false;
};

struct CountMastersGateShard
{
    bool active = false;
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 vel = glm::vec3(0.0f);
    glm::vec3 rot = glm::vec3(0.0f);
    glm::vec3 angVel = glm::vec3(0.0f);
    float age = 0.0f;
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
    static inline constexpr float FORMATION_SPACING_X = 0.135f;
    static inline constexpr float FORMATION_SPACING_Z = 0.155f;
    static inline constexpr float FOLLOW_CATCHUP_SPEED_MPS = RUN_SPEED_MPS * 2.8f;
    static inline constexpr float FOLLOW_SIDE_SPEED_MPS = 0.95f;
    static inline constexpr float GATE_SIDE_CENTER_X = 0.25f;
    static inline constexpr float GATE_APPROACH_WINDOW_M = 0.72f;
    static inline constexpr float GATE_OPEN_SIDE_TOLERANCE_M = 0.11f;
    static inline constexpr int SHARDS_PER_GATE = 14;
    static inline constexpr int MAX_GATE_SHARDS = GATE_COUNT * SHARDS_PER_GATE;
    static inline constexpr float SHARD_LIFETIME_S = 2.0f;

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
    std::array<glm::vec2, MAX_UNITS> units{}; // x,z; unit 0 is the player-controlled leader.
    std::array<CountMastersGateShard, MAX_GATE_SHARDS> gateShards{};

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

    static inline int RowForUnitIndex(int unitIndex)
    {
        int row = 0;
        while ((row + 1) * (row + 2) / 2 <= unitIndex)
            ++row;
        return row;
    }

    static inline glm::vec2 FormationSlotForUnitIndex(int unitIndex, float leaderX, float leaderZ)
    {
        const int row = RowForUnitIndex(unitIndex);
        const int rowStart = row * (row + 1) / 2;
        const int slot = unitIndex - rowStart;
        const float xOffset = (float(slot) - float(row) * 0.5f) * FORMATION_SPACING_X;
        return glm::vec2(
            std::clamp(leaderX + xOffset, -LANE_HALF_WIDTH + 0.035f, LANE_HALF_WIDTH - 0.035f),
            leaderZ + float(row) * FORMATION_SPACING_Z
        );
    }

    void syncUnitCount(int oldCount, int newCount)
    {
        oldCount = std::clamp(oldCount, 0, MAX_UNITS);
        newCount = std::clamp(newCount, 0, MAX_UNITS);
        if (oldCount <= 0 && newCount > 0)
        {
            units[0] = glm::vec2(runnerX, runnerZ);
            oldCount = 1;
        }
        for (int i = oldCount; i < newCount; ++i)
        {
            const glm::vec2 slot = FormationSlotForUnitIndex(i, runnerX, runnerZ);
            // New clones appear near the leader and then settle into the flock.
            units[i] = glm::vec2(runnerX, runnerZ + 0.05f + float(i - oldCount) * 0.018f);
            units[i].x = std::clamp(units[i].x, -LANE_HALF_WIDTH + 0.035f, LANE_HALF_WIDTH - 0.035f);
            units[i].y = std::min(units[i].y, slot.y + 0.18f);
        }
        playerCount = newCount;
    }

    void assignFollowerSlots(std::array<glm::vec2, MAX_UNITS> &targets) const
    {
        std::array<bool, MAX_UNITS> assigned{};
        targets[0] = glm::vec2(runnerX, runnerZ);
        assigned[0] = true;

        for (int slotIndex = 1; slotIndex < playerCount; ++slotIndex)
        {
            const glm::vec2 slot = FormationSlotForUnitIndex(slotIndex, runnerX, runnerZ);
            int bestUnit = -1;
            float bestScore = 1.0e9f;
            for (int unitIndex = 1; unitIndex < playerCount; ++unitIndex)
            {
                if (assigned[unitIndex])
                    continue;
                const glm::vec2 d = units[unitIndex] - slot;
                // Favor front/back correctness slightly so rows do not churn sideways too much.
                const float score = d.x * d.x + d.y * d.y * 0.55f;
                if (score < bestScore)
                {
                    bestScore = score;
                    bestUnit = unitIndex;
                }
            }
            if (bestUnit >= 0)
            {
                targets[bestUnit] = slot;
                assigned[bestUnit] = true;
            }
        }
    }

    glm::vec2 applyGateFlowTarget(glm::vec2 unitPos, glm::vec2 target) const
    {
        for (const CountMastersGateRow &gate : gates)
        {
            if (!gate.resolved || gate.chosenSide == 0)
                continue;
            if (unitPos.y <= gate.z || unitPos.y > gate.z + GATE_APPROACH_WINDOW_M)
                continue;

            const float openX = float(gate.chosenSide) * GATE_SIDE_CENTER_X;
            target.x = openX;
            if (std::abs(unitPos.x - openX) > GATE_OPEN_SIDE_TOLERANCE_M)
                target.y = std::max(target.y, gate.z + 0.04f);
        }
        target.x = std::clamp(target.x, -LANE_HALF_WIDTH + 0.035f, LANE_HALF_WIDTH - 0.035f);
        return target;
    }

    static inline float MoveTowards(float current, float target, float maxStep)
    {
        const float delta = target - current;
        if (std::abs(delta) <= maxStep)
            return target;
        return current + (delta < 0.0f ? -maxStep : maxStep);
    }

    static inline float HashSigned(int seed)
    {
        uint32_t x = uint32_t(seed) * 747796405u + 2891336453u;
        x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        x = (x >> 22u) ^ x;
        return (float(x & 0xffffu) / 32767.5f) - 1.0f;
    }

    void spawnGateShards(int gateIndex, int chosenSide)
    {
        if (gateIndex < 0 || gateIndex >= GATE_COUNT || chosenSide == 0)
            return;

        const CountMastersGateRow &gate = gates[gateIndex];
        const int base = gateIndex * SHARDS_PER_GATE;
        const float sideX = float(chosenSide) * GATE_SIDE_CENTER_X;
        for (int i = 0; i < SHARDS_PER_GATE; ++i)
        {
            CountMastersGateShard &shard = gateShards[base + i];
            const float row = float(i / 4);
            const float col = float(i % 4);
            const float sx = (col - 1.5f) * 0.075f + HashSigned(gateIndex * 73 + i * 9) * 0.018f;
            const float sy = (row - 1.1f) * 0.105f + HashSigned(gateIndex * 97 + i * 11) * 0.018f;
            shard.active = true;
            shard.pos = glm::vec3(sideX + sx, 0.26f + sy, gate.z);
            shard.vel = glm::vec3(
                float(chosenSide) * (0.16f + 0.11f * std::abs(HashSigned(gateIndex * 31 + i))),
                0.70f + 0.28f * std::abs(HashSigned(gateIndex * 41 + i)),
                -0.34f - 0.20f * std::abs(HashSigned(gateIndex * 53 + i))
            );
            shard.rot = glm::vec3(
                HashSigned(gateIndex * 59 + i) * 1.3f,
                HashSigned(gateIndex * 61 + i) * 1.3f,
                HashSigned(gateIndex * 67 + i) * 1.3f
            );
            shard.angVel = glm::vec3(
                HashSigned(gateIndex * 71 + i) * 7.0f,
                HashSigned(gateIndex * 79 + i) * 8.0f,
                HashSigned(gateIndex * 83 + i) * 7.0f
            );
            shard.age = 0.0f;
        }
    }

    void updateGateShards(float dt)
    {
        for (CountMastersGateShard &shard : gateShards)
        {
            if (!shard.active)
                continue;
            shard.age += dt;
            if (shard.age >= SHARD_LIFETIME_S)
            {
                shard.active = false;
                continue;
            }

            shard.vel.y -= 2.8f * dt;
            shard.pos += shard.vel * dt;
            shard.rot += shard.angVel * dt;
            if (shard.pos.y < 0.035f)
            {
                shard.pos.y = 0.035f;
                if (std::abs(shard.vel.y) > 0.10f)
                    shard.vel.y = -shard.vel.y * 0.34f;
                else
                    shard.vel.y = 0.0f;
                shard.vel.x *= 0.86f;
                shard.vel.z *= 0.86f;
                shard.angVel *= 0.82f;
            }
        }
    }

    void updateFollowerUnits(float dt)
    {
        if (playerCount <= 0)
            return;

        units[0] = glm::vec2(runnerX, runnerZ);
        std::array<glm::vec2, MAX_UNITS> targets{};
        assignFollowerSlots(targets);

        for (int i = 1; i < playerCount; ++i)
        {
            glm::vec2 target = applyGateFlowTarget(units[i], targets[i]);
            units[i].x = MoveTowards(units[i].x, target.x, FOLLOW_SIDE_SPEED_MPS * dt);
            units[i].y = MoveTowards(units[i].y, target.y, FOLLOW_CATCHUP_SPEED_MPS * dt);
            units[i].x = std::clamp(units[i].x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        }
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
        units.fill(glm::vec2(0.0f));
        units[0] = glm::vec2(runnerX, runnerZ);
        gateShards.fill({});

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
        runnerX = std::clamp(runnerX, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        runnerZ -= RUN_SPEED_MPS * dt;
        units[0] = glm::vec2(runnerX, runnerZ);

        for (int gateIndex = 0; gateIndex < GATE_COUNT; ++gateIndex)
        {
            CountMastersGateRow &gate = gates[gateIndex];
            if (gate.resolved || runnerZ > gate.z)
                continue;
            gate.resolved = true;
            gate.chosenSide = (runnerX < 0.0f) ? -1 : 1;
            spawnGateShards(gateIndex, gate.chosenSide);
            const CountMastersGateChoice choice = (gate.chosenSide < 0) ? gate.left : gate.right;
            const int oldCount = playerCount;
            const int newCount = ApplyGateMath(playerCount, choice);
            syncUnitCount(oldCount, newCount);
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
            syncUnitCount(playersBeforeFight, ResolveFight(playersBeforeFight, enemiesBeforeFight));
            enemy.count = std::max(0, enemiesBeforeFight - playersBeforeFight);
            if (playerCount <= 0)
            {
                phase = CountMastersPhase::LOST;
                return;
            }
        }

        updateFollowerUnits(dt);
        updateGateShards(dt);

        if (runnerZ <= FINISH_Z)
        {
            pinsHit = std::min(PIN_COUNT, playerCount);
            rewardCoins = ComputeRewardCoins(playerCount);
            phase = (playerCount > 0) ? CountMastersPhase::WON : CountMastersPhase::LOST;
        }
    }
};
