#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <glm/glm.hpp>

#include "../minigame_sfx_events.h"

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

enum class CountMastersUnitMode : uint8_t
{
    Moving = 0,
    Fighting = 1,
    Dead = 2,
};

struct CountMastersGateRow
{
    float z = 0.0f;
    CountMastersGateChoice left;
    CountMastersGateChoice right;
    bool resolved = false;
    int chosenSide = 0; // -1 left, +1 right, 0 unresolved
    int leftCoinCount = 1;
    int rightCoinCount = 1;
};

struct CountMastersEnemySquad
{
    float z = 0.0f;
    int count = 0;
    bool resolved = false;
    bool engaged = false;
    bool leaderReachedCenter = false;
    glm::vec2 center = glm::vec2(0.0f);
    std::array<glm::vec2, 64> units{};
    std::array<glm::vec2, 64> home{};
    std::array<CountMastersUnitMode, 64> modes{};
    std::array<int, 64> targetPlayer{};
    std::array<float, 64> fightTime{};
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

struct CountMastersPinState
{
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 vel = glm::vec2(0.0f);
    glm::vec2 tiltDir = glm::vec2(0.0f, 1.0f);
    float tilt = 0.0f;
    float tiltVel = 0.0f;
    float notUprightTime = 0.0f;
    bool down = false;
};

struct CountMastersDeathFx
{
    bool active = false;
    bool malach = false;
    glm::vec2 startPos = glm::vec2(0.0f);
    glm::vec2 flyDir = glm::vec2(1.0f, 0.0f);
    float age = 0.0f;
    float delay = 0.0f;
    float duration = 0.55f;
    float distance = 1.2f;
    float arcHeight = 0.45f;
    float spin = 0.0f;
};

enum class CountMastersPhase : uint8_t
{
    INACTIVE = 0,
    RUNNING = 1,
    PIN_CRASH = 2,
    WON = 3,
    LOST = 4,
};

struct CountMastersState
{
    static inline constexpr int MAX_UNITS = 64;
    static inline constexpr int GATE_COUNT = 4;
    static inline constexpr int ENEMY_SQUAD_COUNT = 4;
    static inline constexpr int PIN_COUNT = 10;
    static inline constexpr int MOTION_HISTORY_COUNT = 32;
    static inline constexpr int MAX_DEATH_FX = 128;
    static inline constexpr int MAX_SFX_EVENTS = 64;
    static inline constexpr int MAX_PARTICLE_EVENTS = 96;
    static inline constexpr int MAX_GATE_COINS_PER_SIDE = 5;
    static inline constexpr float LANE_HALF_WIDTH = 0.50f;
    static inline constexpr float START_Z = -15.15f;
    static inline constexpr float FINISH_Z = -1.10f;
    static inline constexpr float RUN_SPEED_MPS = 0.61250f;
    static inline constexpr float RUN_ANIM_PLAYBACK_SCALE = 2.0f;
    static inline constexpr float SIDE_FOLLOW_SPEED = 8.0f;
    static inline constexpr float FORMATION_SPACING_X = 0.085f;
    static inline constexpr float FORMATION_SPACING_Z = 0.085f;
    static inline constexpr float FORMATION_MIN_SEPARATION_M = 0.070f;
    static inline constexpr float FOLLOW_CATCHUP_SPEED_MPS = RUN_SPEED_MPS * 2.8f;
    static inline constexpr float FOLLOW_SIDE_SPEED_MPS = 0.95f;
    static inline constexpr float GATE_SIDE_CENTER_X = 0.25f;
    static inline constexpr float GATE_APPROACH_WINDOW_M = 0.72f;
    static inline constexpr float GATE_OPEN_SIDE_TOLERANCE_M = 0.11f;
    static inline constexpr int SHARDS_PER_GATE = 14;
    static inline constexpr int MAX_GATE_SHARDS = GATE_COUNT * SHARDS_PER_GATE;
    static inline constexpr float SHARD_LIFETIME_S = 2.0f;
    static inline constexpr float FIGHT_DURATION_S = 1.00f;
    static inline constexpr float FIGHT_TRIGGER_DISTANCE_M = FORMATION_MIN_SEPARATION_M * 1.20f;
    static inline constexpr float FIGHT_LEADER_CENTER_RADIUS_M = 0.11f;
    static inline constexpr float FIGHTER_SPEED_MPS = 0.82f;
    static inline constexpr float ENEMY_HOLD_SPEED_MPS = 0.36f;
    static inline constexpr float FIGHT_DEPLOY_DURATION_S = 0.35f;
    static inline constexpr float PIN_DIRECTION_MEMORY_S = 0.40f;
    static inline constexpr float PIN_MEMBER_RADIUS_M = 0.045f;
    static inline constexpr float PIN_MEMBER_PHYSICS_RADIUS_M = 0.033f;
    static inline constexpr float PIN_RADIUS_M = 0.050f;
    static inline constexpr float PIN_CENTER_Y = 0.19f;
    static inline constexpr float PIN_RACK_SPACING_M = 0.305f;
    static inline constexpr float PIN_RACK_ROW_SPACING_M = PIN_RACK_SPACING_M * 0.86602540378f;
    static inline constexpr float PIN_RACK_BACK_Z = 0.75f;
    static inline constexpr float PIN_RACK_FRONT_Z = PIN_RACK_BACK_Z - PIN_RACK_ROW_SPACING_M * 3.0f;
    static inline constexpr float PIN_DECK_END_Z = PIN_RACK_BACK_Z + 0.12f;
    static inline constexpr float PIN_CRASH_LANE_MARGIN_M = 0.04f;
    static inline constexpr float PIN_CRASH_RESULT_HOLD_S = 1.0f;
    static inline constexpr float PIN_DOWN_DOT_THRESHOLD = 0.85f;
    static inline constexpr float PIN_DOWN_CONFIRM_S = 0.18f;

    CountMastersPhase phase = CountMastersPhase::INACTIVE;
    float runnerX = 0.0f;
    float targetX = 0.0f;
    float runnerZ = START_Z;
    float elapsed = 0.0f;
    int playerCount = 1;
    int pinsHit = 0;
    int standers = 0;
    int rewardCoins = 0;
    int gateCoinsCollected = 0;
    int activeFightSquad = -1;
    bool waitingForFirstInput = false;
    std::array<CountMastersGateRow, GATE_COUNT> gates{};
    std::array<CountMastersEnemySquad, ENEMY_SQUAD_COUNT> enemies{};
    std::array<glm::vec2, MAX_UNITS> units{}; // x,z; unit 0 is the player-controlled leader.
    std::array<CountMastersUnitMode, MAX_UNITS> unitModes{};
    std::array<int, MAX_UNITS> unitTargetEnemy{};
    std::array<float, MAX_UNITS> unitFightTime{};
    std::array<CountMastersGateShard, MAX_GATE_SHARDS> gateShards{};
    bool fightDeploymentActive = false;
    float fightDeploymentTime = 0.0f;
    std::array<bool, MAX_UNITS> unitDeployActive{};
    std::array<glm::vec2, MAX_UNITS> unitDeployStart{};
    std::array<glm::vec2, MAX_UNITS> unitDeployTarget{};
    std::array<bool, 64> enemyDeployActive{};
    std::array<glm::vec2, 64> enemyDeployStart{};
    std::array<glm::vec2, 64> enemyDeployTarget{};
    std::array<CountMastersPinState, PIN_COUNT> pins{};
    glm::vec2 pinCrashCenter = glm::vec2(0.0f);
    glm::vec2 pinCrashVelocity = glm::vec2(0.0f, RUN_SPEED_MPS);
    bool pinCrashNeedsPhysicsStart = false;
    bool pinCrashPhysicsStarted = false;
    bool pinCrashScoringComplete = false;
    float pinCrashResultHoldTime = 0.0f;
    std::array<glm::vec2, MOTION_HISTORY_COUNT> motionHistoryPos{};
    std::array<float, MOTION_HISTORY_COUNT> motionHistoryTime{};
    int motionHistoryWrite = 0;
    int motionHistoryUsed = 0;
    std::array<CountMastersDeathFx, MAX_DEATH_FX> deathFx{};
    MiniGameSfxEventQueue<MAX_SFX_EVENTS> sfxEvents{};
    MiniGameParticleEventQueue<MAX_PARTICLE_EVENTS> particleEvents{};
    int deathFxCursor = 0;
    uint32_t deathFxSeed = 0xA341316Cu;

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

    static inline uint32_t NextGateSeed(uint32_t &seed)
    {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    }

    float nextDeathFxRand01()
    {
        return float(NextGateSeed(deathFxSeed) & 0x00FFFFFFu) / float(0x01000000u);
    }

    static inline float DeathMiddleDirectness(float startX, glm::vec2 flyDir)
    {
        const float towardMiddleX = std::abs(startX) > 1.0e-4f
            ? -std::copysign(1.0f, startX)
            : 0.0f;
        return std::clamp(flyDir.x * towardMiddleX, 0.0f, 1.0f);
    }

    static inline float DeathArcHeightMultiplier(float startX, glm::vec2 flyDir)
    {
        return 1.0f + DeathMiddleDirectness(startX, flyDir);
    }

    void spawnDeathFx(glm::vec2 pos, bool malach, glm::vec2 lastMove)
    {
        sfxEvents.push(malach ? MiniGameSfxEvent::ANGEL_DIED : MiniGameSfxEvent::ENEMY_DIED);
        spawnParticleEvent(
            malach ? MiniGameParticleEventKind::ANGEL_DIED : MiniGameParticleEventKind::ENEMY_DIED,
            pos,
            lastMove,
            0.62f
        );
        CountMastersDeathFx &fx = deathFx[deathFxCursor];
        deathFxCursor = (deathFxCursor + 1) % MAX_DEATH_FX;
        fx = CountMastersDeathFx{};
        fx.active = true;
        fx.malach = malach;
        fx.startPos = pos;
        fx.delay = nextDeathFxRand01() * 0.40f;
        fx.duration = 0.52f + nextDeathFxRand01() * 0.18f;
        fx.distance = 1.0f + nextDeathFxRand01() * 0.50f;
        fx.arcHeight = 0.32f + nextDeathFxRand01() * 0.30f;

        const float side = (pos.x >= 0.0f ? 1.0f : -1.0f) * (nextDeathFxRand01() < 0.35f ? -1.0f : 1.0f);
        fx.spin = (4.5f + nextDeathFxRand01() * 4.5f) * side;

        glm::vec2 opposite = -lastMove;
        if (opposite.x * opposite.x + opposite.y * opposite.y < 1.0e-5f)
            opposite = malach ? glm::vec2(0.0f, -1.0f) : glm::vec2(0.0f, 1.0f);
        glm::vec2 dir(side * (0.95f + nextDeathFxRand01() * 0.55f), opposite.y * 0.45f + opposite.x * 0.12f);
        const float len2 = dir.x * dir.x + dir.y * dir.y;
        fx.flyDir = len2 > 1.0e-5f ? dir / std::sqrt(len2) : glm::vec2(side, 0.0f);
        fx.arcHeight *= DeathArcHeightMultiplier(pos.x, fx.flyDir);
    }

    void spawnParticleEvent(MiniGameParticleEventKind kind, glm::vec2 pos, glm::vec2 dir, float intensity)
    {
        MiniGameParticleEvent event{};
        event.kind = kind;
        event.pos = pos;
        event.dir = dir;
        event.intensity = intensity;
        particleEvents.push(event);
    }

    void updateDeathFx(float dt)
    {
        dt = std::clamp(dt, 0.0f, 0.05f);
        for (CountMastersDeathFx &fx : deathFx)
        {
            if (!fx.active)
                continue;
            if (fx.delay > 0.0f)
            {
                fx.delay = std::max(0.0f, fx.delay - dt);
                continue;
            }
            fx.age += dt;
            if (fx.age >= fx.duration)
                fx = CountMastersDeathFx{};
        }
    }

    static inline CountMastersGateRow MakeVariedGateRow(int gateIndex, float z, uint32_t &seed)
    {
        static constexpr CountMastersGateChoice kChoices[GATE_COUNT][8][2] = {
            {
                {{CountMastersOp::ADD, 5}, {CountMastersOp::MULTIPLY, 3}},
                {{CountMastersOp::ADD, 4}, {CountMastersOp::MULTIPLY, 2}},
                {{CountMastersOp::ADD, 8}, {CountMastersOp::MULTIPLY, 2}},
                {{CountMastersOp::SUBTRACT, 1}, {CountMastersOp::ADD, 10}},
                {{CountMastersOp::ADD, 6}, {CountMastersOp::MULTIPLY, 4}},
                {{CountMastersOp::DIVIDE, 2}, {CountMastersOp::ADD, 12}},
                {{CountMastersOp::ADD, 3}, {CountMastersOp::MULTIPLY, 5}},
                {{CountMastersOp::SUBTRACT, 2}, {CountMastersOp::ADD, 14}},
            },
            {
                {{CountMastersOp::MULTIPLY, 2}, {CountMastersOp::ADD, 12}},
                {{CountMastersOp::ADD, 9}, {CountMastersOp::MULTIPLY, 3}},
                {{CountMastersOp::DIVIDE, 2}, {CountMastersOp::ADD, 18}},
                {{CountMastersOp::MULTIPLY, 4}, {CountMastersOp::SUBTRACT, 3}},
                {{CountMastersOp::ADD, 7}, {CountMastersOp::MULTIPLY, 5}},
                {{CountMastersOp::SUBTRACT, 5}, {CountMastersOp::ADD, 20}},
                {{CountMastersOp::MULTIPLY, 3}, {CountMastersOp::ADD, 11}},
                {{CountMastersOp::DIVIDE, 3}, {CountMastersOp::MULTIPLY, 4}},
            },
            {
                {{CountMastersOp::SUBTRACT, 4}, {CountMastersOp::MULTIPLY, 5}},
                {{CountMastersOp::ADD, 15}, {CountMastersOp::MULTIPLY, 2}},
                {{CountMastersOp::DIVIDE, 2}, {CountMastersOp::MULTIPLY, 4}},
                {{CountMastersOp::ADD, 20}, {CountMastersOp::SUBTRACT, 6}},
                {{CountMastersOp::MULTIPLY, 3}, {CountMastersOp::ADD, 16}},
                {{CountMastersOp::SUBTRACT, 8}, {CountMastersOp::MULTIPLY, 6}},
                {{CountMastersOp::ADD, 10}, {CountMastersOp::DIVIDE, 2}},
                {{CountMastersOp::MULTIPLY, 2}, {CountMastersOp::ADD, 22}},
            },
            {
                {{CountMastersOp::DIVIDE, 2}, {CountMastersOp::ADD, 20}},
                {{CountMastersOp::ADD, 24}, {CountMastersOp::MULTIPLY, 2}},
                {{CountMastersOp::MULTIPLY, 3}, {CountMastersOp::SUBTRACT, 8}},
                {{CountMastersOp::DIVIDE, 3}, {CountMastersOp::ADD, 30}},
                {{CountMastersOp::ADD, 12}, {CountMastersOp::MULTIPLY, 4}},
                {{CountMastersOp::SUBTRACT, 10}, {CountMastersOp::ADD, 28}},
                {{CountMastersOp::MULTIPLY, 2}, {CountMastersOp::DIVIDE, 2}},
                {{CountMastersOp::ADD, 18}, {CountMastersOp::MULTIPLY, 3}},
            },
        };

        const uint32_t pick = NextGateSeed(seed) % 8u;
        CountMastersGateRow row = {z, kChoices[gateIndex][pick][0], kChoices[gateIndex][pick][1], false};
        if ((NextGateSeed(seed) & 1u) != 0u)
            std::swap(row.left, row.right);
        row.leftCoinCount = 1 + int(NextGateSeed(seed) % uint32_t(MAX_GATE_COINS_PER_SIDE));
        row.rightCoinCount = 1 + int(NextGateSeed(seed) % uint32_t(MAX_GATE_COINS_PER_SIDE));
        return row;
    }

    static inline int ResolveFight(int players, int enemies)
    {
        return std::max(0, players - std::max(0, enemies));
    }

    static inline int ComputeRewardCoins(int pinsDown, int standingMembers)
    {
        return std::clamp(pinsDown, 0, PIN_COUNT) * 10 + std::max(0, standingMembers);
    }

    static inline glm::vec2 PinPositionForIndex(int index)
    {
        static constexpr int kRowStart[4] = {0, 1, 3, 6};
        const int clamped = std::clamp(index, 0, PIN_COUNT - 1);
        int row = 0;
        while (row + 1 < 4 && clamped >= kRowStart[row + 1])
            ++row;
        const int col = clamped - kRowStart[row];
        const float x = (float(col) - float(row) * 0.5f) * PIN_RACK_SPACING_M;
        const float z = PIN_RACK_FRONT_Z + float(row) * PIN_RACK_ROW_SPACING_M;
        return glm::vec2(x, z);
    }

    static inline bool IsPinUpright(float tilt)
    {
        return std::cos(tilt) > PIN_DOWN_DOT_THRESHOLD;
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

    static inline int RingForUnitIndex(int unitIndex)
    {
        if (unitIndex <= 0)
            return 0;
        int ring = 1;
        int firstAfterRing = 1 + 3 * ring * (ring + 1);
        while (unitIndex >= firstAfterRing)
        {
            ++ring;
            firstAfterRing = 1 + 3 * ring * (ring + 1);
        }
        return ring;
    }

    static inline glm::vec2 FormationSlotForUnitIndex(int unitIndex, float leaderX, float leaderZ)
    {
        if (unitIndex <= 0)
            return glm::vec2(leaderX, leaderZ);

        const int ring = RingForUnitIndex(unitIndex);
        const int ringStart = 1 + 3 * (ring - 1) * ring;
        const int slot = unitIndex - ringStart;
        const int slotsInRing = 6 * ring;
        const float angle = -1.57079632679f + (float(slot) + 0.5f * float(ring & 1)) *
            (6.28318530718f / float(slotsInRing));
        const float radius = float(ring) * FORMATION_SPACING_X;
        return glm::vec2(
            std::clamp(leaderX + std::cos(angle) * radius, -LANE_HALF_WIDTH + 0.035f, LANE_HALF_WIDTH - 0.035f),
            leaderZ + std::sin(angle) * radius
        );
    }

    static inline glm::vec2 UnclampedFormationSlotForUnitIndex(int unitIndex, float leaderX, float leaderZ)
    {
        if (unitIndex <= 0)
            return glm::vec2(leaderX, leaderZ);

        const int ring = RingForUnitIndex(unitIndex);
        const int ringStart = 1 + 3 * (ring - 1) * ring;
        const int slot = unitIndex - ringStart;
        const int slotsInRing = 6 * ring;
        const float angle = -1.57079632679f + (float(slot) + 0.5f * float(ring & 1)) *
            (6.28318530718f / float(slotsInRing));
        const float radius = float(ring) * FORMATION_SPACING_X;
        return glm::vec2(leaderX + std::cos(angle) * radius, leaderZ + std::sin(angle) * radius);
    }

    static inline glm::vec2 FightSideSlotForOrdinal(int ordinal, bool playerSide, float centerX, float centerZ)
    {
        ordinal = std::max(0, ordinal);
        int found = 0;
        for (int slotIndex = 1; slotIndex < MAX_UNITS * 4; ++slotIndex)
        {
            const glm::vec2 slot = FormationSlotForUnitIndex(slotIndex, centerX, centerZ);
            const bool onSide = playerSide ? (slot.y <= centerZ + 0.0001f) : (slot.y > centerZ + 0.0001f);
            if (!onSide)
                continue;
            if (found++ == ordinal)
                return slot;
        }

        const float fallbackSign = playerSide ? 1.0f : -1.0f;
        return glm::vec2(
            std::clamp(centerX, -LANE_HALF_WIDTH + 0.035f, LANE_HALF_WIDTH - 0.035f),
            centerZ + fallbackSign * FORMATION_SPACING_Z * float(ordinal + 1)
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
        for (int i = newCount; i < oldCount; ++i)
        {
            units[i] = glm::vec2(0.0f);
            unitModes[i] = CountMastersUnitMode::Dead;
            unitTargetEnemy[i] = -1;
            unitFightTime[i] = 0.0f;
        }
        for (int i = oldCount; i < newCount; ++i)
        {
            // Gate rewards are instantaneous: new units must already be in formation before combat starts.
            units[i] = FormationSlotForUnitIndex(i, runnerX, runnerZ);
            unitModes[i] = CountMastersUnitMode::Moving;
            unitTargetEnemy[i] = -1;
            unitFightTime[i] = 0.0f;
        }
        playerCount = newCount;
    }

    static inline glm::vec2 EnemySlotForIndex(int unitIndex, float centerZ)
    {
        return FormationSlotForUnitIndex(unitIndex, 0.0f, centerZ);
    }

    static inline void InitEnemySquadUnits(CountMastersEnemySquad &enemy)
    {
        enemy.resolved = false;
        enemy.engaged = false;
        enemy.leaderReachedCenter = false;
        enemy.center = glm::vec2(0.0f, enemy.z);
        enemy.targetPlayer.fill(-1);
        enemy.fightTime.fill(0.0f);
        for (int i = 0; i < 64; ++i)
        {
            enemy.home[i] = EnemySlotForIndex(i, enemy.z);
            enemy.units[i] = enemy.home[i];
            enemy.modes[i] = (i < enemy.count) ? CountMastersUnitMode::Moving : CountMastersUnitMode::Dead;
        }
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
            if (unitPos.y >= gate.z || unitPos.y < gate.z - GATE_APPROACH_WINDOW_M)
                continue;

            const float openX = float(gate.chosenSide) * GATE_SIDE_CENTER_X;
            target.x = openX;
            if (std::abs(unitPos.x - openX) > GATE_OPEN_SIDE_TOLERANCE_M)
            {
                // Wide circular crowds must first clear sideways; otherwise followers can clog the leader's gate.
                target.y = std::min(target.y, gate.z - 0.04f);
            }
            else
            {
                target.y = std::max(target.y, gate.z + 0.06f);
            }
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

    static inline glm::vec2 MovePointTowards(glm::vec2 current, glm::vec2 target, float maxStep)
    {
        current.x = MoveTowards(current.x, target.x, maxStep);
        current.y = MoveTowards(current.y, target.y, maxStep);
        return current;
    }

    static inline float DistancePointToSegment(glm::vec2 p, glm::vec2 a, glm::vec2 b)
    {
        const glm::vec2 ab = b - a;
        const float len2 = ab.x * ab.x + ab.y * ab.y;
        if (len2 <= 0.000001f)
        {
            const glm::vec2 d = p - a;
            return std::sqrt(d.x * d.x + d.y * d.y);
        }
        const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2, 0.0f, 1.0f);
        const glm::vec2 closest = a + ab * t;
        const glm::vec2 d = p - closest;
        return std::sqrt(d.x * d.x + d.y * d.y);
    }

    static inline void PushApart(glm::vec2 &a, glm::vec2 &b, float minDistance, float aWeight = 0.5f, float bWeight = 0.5f)
    {
        glm::vec2 d = b - a;
        float dist2 = d.x * d.x + d.y * d.y;
        if (dist2 >= minDistance * minDistance)
            return;
        if (dist2 < 0.000001f)
        {
            d = glm::vec2(1.0f, 0.0f);
            dist2 = 1.0f;
        }
        const float dist = std::sqrt(dist2);
        const glm::vec2 n = d / dist;
        const float push = (minDistance - dist);
        a -= n * push * aWeight;
        b += n * push * bWeight;
        a.x = std::clamp(a.x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        b.x = std::clamp(b.x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
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

    int activeEnemyCount(const CountMastersEnemySquad &enemy) const
    {
        int count = 0;
        for (int i = 0; i < enemy.count; ++i)
            if (enemy.modes[i] != CountMastersUnitMode::Dead)
                ++count;
        return count;
    }

    int activePlayerCount() const
    {
        int count = 0;
        for (int i = 0; i < playerCount; ++i)
            if (unitModes[i] != CountMastersUnitMode::Dead)
                ++count;
        return count;
    }

    int standingPlayerCount() const
    {
        int count = 0;
        for (int i = 0; i < playerCount; ++i)
        {
            if (unitModes[i] != CountMastersUnitMode::Dead &&
                std::abs(units[i].x) <= LANE_HALF_WIDTH + PIN_CRASH_LANE_MARGIN_M)
            {
                ++count;
            }
        }
        return count;
    }

    void resetPins()
    {
        for (int i = 0; i < PIN_COUNT; ++i)
        {
            pins[i] = {};
            pins[i].pos = PinPositionForIndex(i);
            pins[i].tiltDir = glm::vec2(0.0f, 1.0f);
        }
    }

    bool anyMovingPlayerTouchesStandingPin() const
    {
        const float contact = PIN_MEMBER_RADIUS_M + PIN_RADIUS_M;
        const float contact2 = contact * contact;
        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] == CountMastersUnitMode::Dead)
                continue;
            for (int pin = 0; pin < PIN_COUNT; ++pin)
            {
                if (pins[pin].down)
                    continue;
                const glm::vec2 d = units[p] - pins[pin].pos;
                if (d.x * d.x + d.y * d.y <= contact2)
                    return true;
            }
        }
        return false;
    }

    int standingPinCount() const
    {
        int count = 0;
        for (const CountMastersPinState &pin : pins)
            count += pin.down ? 0 : 1;
        return count;
    }

    void recordMotionHistory()
    {
        motionHistoryPos[motionHistoryWrite] = glm::vec2(runnerX, runnerZ);
        motionHistoryTime[motionHistoryWrite] = elapsed;
        motionHistoryWrite = (motionHistoryWrite + 1) % MOTION_HISTORY_COUNT;
        motionHistoryUsed = std::min(motionHistoryUsed + 1, MOTION_HISTORY_COUNT);
    }

    glm::vec2 computeCapturedPinCrashVelocity() const
    {
        const glm::vec2 current(runnerX, runnerZ);
        glm::vec2 oldest = current;
        float oldestTime = elapsed;
        const float cutoff = elapsed - PIN_DIRECTION_MEMORY_S;
        bool found = false;
        for (int i = 0; i < motionHistoryUsed; ++i)
        {
            const float sampleTime = motionHistoryTime[i];
            if (sampleTime < cutoff || sampleTime > elapsed)
                continue;
            if (!found || sampleTime < oldestTime)
            {
                oldestTime = sampleTime;
                oldest = motionHistoryPos[i];
                found = true;
            }
        }

        glm::vec2 velocity = found ? (current - oldest) / std::max(0.001f, elapsed - oldestTime) : glm::vec2(0.0f, RUN_SPEED_MPS);
        const float speed2 = velocity.x * velocity.x + velocity.y * velocity.y;
        if (speed2 < RUN_SPEED_MPS * RUN_SPEED_MPS * 0.25f)
            velocity = glm::vec2(0.0f, RUN_SPEED_MPS);
        else
        {
            const float speed = std::sqrt(speed2);
            const float clampedSpeed = std::clamp(speed, RUN_SPEED_MPS * 0.85f, RUN_SPEED_MPS * 3.25f);
            velocity *= clampedSpeed / speed;
        }
        return velocity;
    }

    void beginPinCrash()
    {
        activeFightSquad = -1;
        clearFightDeployment();
        compactLivePlayers();
        pinCrashCenter = glm::vec2(runnerX, runnerZ);
        pinCrashVelocity = computeCapturedPinCrashVelocity();
        pinCrashNeedsPhysicsStart = true;
        pinCrashPhysicsStarted = false;
        pinCrashScoringComplete = false;
        pinCrashResultHoldTime = 0.0f;
        phase = CountMastersPhase::PIN_CRASH;
    }

    void markPinCrashPhysicsStarted()
    {
        pinCrashNeedsPhysicsStart = false;
        pinCrashPhysicsStarted = true;
    }

    void knockPin(CountMastersPinState &pin, glm::vec2 hitVelocity, glm::vec2 hitNormal)
    {
        const float speed = std::sqrt(hitVelocity.x * hitVelocity.x + hitVelocity.y * hitVelocity.y);
        if (speed <= 0.0001f)
            return;
        if (hitNormal.x * hitNormal.x + hitNormal.y * hitNormal.y <= 0.000001f)
            hitNormal = glm::normalize(hitVelocity);
        pin.vel += hitVelocity * 0.85f + hitNormal * 0.10f;
        pin.tiltDir = glm::normalize(hitVelocity);
        pin.tiltVel = std::max(pin.tiltVel, 3.6f + speed * 2.1f);
    }

    void updatePinBodies(float dt)
    {
        for (CountMastersPinState &pin : pins)
        {
            if (pin.down)
                continue;

            pin.pos += pin.vel * dt;
            pin.vel *= std::pow(0.18f, dt);
            pin.tilt += pin.tiltVel * dt;
            pin.tiltVel *= std::pow(0.28f, dt);
            pin.tilt = std::clamp(pin.tilt, 0.0f, 1.57079632679f);

            const bool offLane =
                std::abs(pin.pos.x) > LANE_HALF_WIDTH + PIN_CRASH_LANE_MARGIN_M ||
                pin.pos.y > PIN_RACK_BACK_Z + 0.85f ||
                pin.pos.y < FINISH_Z - 0.35f;
            if (offLane)
            {
                pin.down = true;
                continue;
            }

            if (IsPinUpright(pin.tilt))
                pin.notUprightTime = 0.0f;
            else
                pin.notUprightTime += dt;

            if (pin.notUprightTime >= PIN_DOWN_CONFIRM_S)
                pin.down = true;
        }
    }

    void finalizePinCrashScore()
    {
        if (pinCrashScoringComplete)
            return;
        standers = standingPlayerCount();
        rewardCoins = ComputeRewardCoins(pinsHit, standers);
        pinCrashScoringComplete = true;
        pinCrashResultHoldTime = 0.0f;
    }

    void syncPinCrashFromPhysics(int physicsPinsDown, int physicsMalachimAlive, const glm::vec2 *physicsMalachim, float dt)
    {
        if (phase != CountMastersPhase::PIN_CRASH || !pinCrashPhysicsStarted)
            return;

        physicsPinsDown = std::clamp(physicsPinsDown, 0, PIN_COUNT);
        physicsMalachimAlive = std::clamp(physicsMalachimAlive, 0, playerCount);
        for (int i = 0; i < playerCount; ++i)
        {
            if (i < physicsMalachimAlive)
            {
                unitModes[i] = CountMastersUnitMode::Moving;
                if (physicsMalachim)
                    units[i] = physicsMalachim[i];
            }
            else
            {
                unitModes[i] = CountMastersUnitMode::Dead;
            }
        }
        if (physicsMalachimAlive > 0 && physicsMalachim)
        {
            runnerX = physicsMalachim[0].x;
            runnerZ = physicsMalachim[0].y;
            targetX = runnerX;
        }

        pinsHit = physicsPinsDown;
        if (!pinCrashScoringComplete && (physicsPinsDown >= PIN_COUNT || physicsMalachimAlive <= 0))
        {
            standers = physicsPinsDown >= PIN_COUNT ? physicsMalachimAlive : 0;
            rewardCoins = ComputeRewardCoins(pinsHit, standers);
            pinCrashScoringComplete = true;
            pinCrashResultHoldTime = 0.0f;
        }

        if (pinCrashScoringComplete)
        {
            pinCrashResultHoldTime += dt;
            if (pinCrashResultHoldTime >= PIN_CRASH_RESULT_HOLD_S)
                phase = rewardCoins > 0 ? CountMastersPhase::WON : CountMastersPhase::LOST;
        }
    }

    void updatePinCrash(float dt)
    {
        updatePinBodies(dt);

        if (pinCrashScoringComplete)
        {
            pinCrashResultHoldTime += dt;
            if (pinCrashResultHoldTime >= PIN_CRASH_RESULT_HOLD_S)
                phase = rewardCoins > 0 ? CountMastersPhase::WON : CountMastersPhase::LOST;
            return;
        }

        const float memberContact = PIN_MEMBER_RADIUS_M + PIN_RADIUS_M;
        for (int i = 0; i < playerCount; ++i)
        {
            if (unitModes[i] == CountMastersUnitMode::Dead)
                continue;

            const glm::vec2 prevUnit = units[i];
            units[i] += pinCrashVelocity * dt;
            if (std::abs(units[i].x) > LANE_HALF_WIDTH + PIN_CRASH_LANE_MARGIN_M ||
                units[i].y > PIN_RACK_BACK_Z + 0.70f)
            {
                unitModes[i] = CountMastersUnitMode::Dead;
                continue;
            }

            for (int pinIndex = 0; pinIndex < PIN_COUNT; ++pinIndex)
            {
                CountMastersPinState &pin = pins[pinIndex];
                if (pin.down)
                    continue;

                const float distance = DistancePointToSegment(pin.pos, prevUnit, units[i]);
                if (distance > memberContact)
                    continue;

                glm::vec2 normal = pin.pos - units[i];
                if (normal.x * normal.x + normal.y * normal.y <= 0.000001f)
                    normal = glm::normalize(pinCrashVelocity);
                else
                    normal = glm::normalize(normal);
                knockPin(pin, pinCrashVelocity, normal);
            }
        }
        pinCrashCenter += pinCrashVelocity * dt;
        runnerX = pinCrashCenter.x;
        runnerZ = pinCrashCenter.y;
        targetX = runnerX;

        pinsHit = PIN_COUNT - standingPinCount();
        if (standingPinCount() == 0 || activePlayerCount() == 0)
        {
            finalizePinCrashScore();
        }
    }

    void compactEnemySquad(CountMastersEnemySquad &enemy)
    {
        std::array<glm::vec2, 64> newUnits{};
        std::array<glm::vec2, 64> newHome{};
        std::array<CountMastersUnitMode, 64> newModes{};
        std::array<int, 64> newTargets{};
        std::array<float, 64> newFightTimes{};
        newTargets.fill(-1);

        int write = 0;
        for (int i = 0; i < enemy.count; ++i)
        {
            if (enemy.modes[i] == CountMastersUnitMode::Dead)
                continue;
            newUnits[write] = enemy.units[i];
            newHome[write] = EnemySlotForIndex(write, enemy.center.y);
            newModes[write] = CountMastersUnitMode::Moving;
            newFightTimes[write] = 0.0f;
            ++write;
        }

        enemy.units = newUnits;
        enemy.home = newHome;
        enemy.modes = newModes;
        enemy.targetPlayer = newTargets;
        enemy.fightTime = newFightTimes;
        enemy.count = write;
    }

    void swapPlayerSlots(int a, int b)
    {
        if (a == b || a < 0 || b < 0 || a >= playerCount || b >= playerCount)
            return;
        std::swap(units[a], units[b]);
        std::swap(unitModes[a], unitModes[b]);
        std::swap(unitTargetEnemy[a], unitTargetEnemy[b]);
        std::swap(unitFightTime[a], unitFightTime[b]);
        for (CountMastersEnemySquad &enemy : enemies)
        {
            for (int i = 0; i < enemy.count; ++i)
            {
                if (enemy.targetPlayer[i] == a)
                    enemy.targetPlayer[i] = b;
                else if (enemy.targetPlayer[i] == b)
                    enemy.targetPlayer[i] = a;
            }
        }
    }

    void electMovingLeaderIfNeeded()
    {
        if (playerCount <= 0)
            return;
        if (unitModes[0] == CountMastersUnitMode::Moving)
        {
            runnerX = units[0].x;
            runnerZ = units[0].y;
            return;
        }

        for (int i = 1; i < playerCount; ++i)
        {
            if (unitModes[i] != CountMastersUnitMode::Moving)
                continue;
            swapPlayerSlots(0, i);
            runnerX = units[0].x;
            runnerZ = units[0].y;
            return;
        }
    }

    void compactLivePlayers()
    {
        std::array<glm::vec2, MAX_UNITS> newUnits{};
        std::array<CountMastersUnitMode, MAX_UNITS> newModes{};
        std::array<int, MAX_UNITS> newTargets{};
        std::array<float, MAX_UNITS> newFightTimes{};
        newTargets.fill(-1);

        int write = 0;
        for (int i = 0; i < playerCount; ++i)
        {
            if (unitModes[i] == CountMastersUnitMode::Dead)
                continue;
            newUnits[write] = units[i];
            newModes[write] = CountMastersUnitMode::Moving;
            newFightTimes[write] = 0.0f;
            ++write;
        }

        units = newUnits;
        unitModes = newModes;
        unitTargetEnemy = newTargets;
        unitFightTime = newFightTimes;
        playerCount = write;
        electMovingLeaderIfNeeded();
    }

    int nearestUntackledEnemyIndex(const CountMastersEnemySquad &enemy, glm::vec2 playerPos, const std::array<bool, 64> &claimed) const
    {
        int best = -1;
        float bestDist2 = 1.0e9f;
        for (int i = 0; i < enemy.count; ++i)
        {
            if (claimed[i] || enemy.modes[i] != CountMastersUnitMode::Moving)
                continue;
            const glm::vec2 d = enemy.units[i] - playerPos;
            const float dist2 = d.x * d.x + d.y * d.y;
            if (dist2 < bestDist2)
            {
                bestDist2 = dist2;
                best = i;
            }
        }
        return best;
    }

    bool firstContactWithEnemy(const CountMastersEnemySquad &enemy, glm::vec2 *outCenter = nullptr) const
    {
        static constexpr float CONTACT_DISTANCE_M = FORMATION_MIN_SEPARATION_M * 1.45f;
        const float contactDist2 = CONTACT_DISTANCE_M * CONTACT_DISTANCE_M;
        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Moving)
                continue;
            for (int e = 0; e < enemy.count; ++e)
            {
                if (enemy.modes[e] != CountMastersUnitMode::Moving)
                    continue;
                const glm::vec2 d = units[p] - enemy.units[e];
                if (d.x * d.x + d.y * d.y <= contactDist2)
                {
                    if (outCenter)
                        *outCenter = (units[p] + enemy.units[e]) * 0.5f;
                    return true;
                }
            }
        }
        return false;
    }

    void assignSharedFightSlots(
        const CountMastersEnemySquad &enemy,
        std::array<glm::vec2, MAX_UNITS> &playerTargets,
        std::array<glm::vec2, 64> &enemyTargets
    ) const
    {
        playerTargets.fill(glm::vec2(runnerX, runnerZ));
        enemyTargets.fill(glm::vec2(runnerX, runnerZ));
        if (playerCount > 0)
            playerTargets[0] = glm::vec2(runnerX, runnerZ);

        std::array<int, MAX_UNITS> movingPlayers{};
        std::array<int, 64> movingEnemies{};
        int movingPlayerCount = 0;
        int movingEnemyCount = 0;

        for (int p = 1; p < playerCount; ++p)
        {
            if (unitModes[p] == CountMastersUnitMode::Moving)
                movingPlayers[movingPlayerCount++] = p;
        }
        for (int e = 0; e < enemy.count; ++e)
        {
            if (enemy.modes[e] == CountMastersUnitMode::Moving)
                movingEnemies[movingEnemyCount++] = e;
        }

        for (int i = 0; i < movingPlayerCount; ++i)
        {
            playerTargets[movingPlayers[i]] = FightSideSlotForOrdinal(i, true, runnerX, runnerZ);
        }
        for (int i = 0; i < movingEnemyCount; ++i)
        {
            enemyTargets[movingEnemies[i]] = FightSideSlotForOrdinal(i, false, runnerX, runnerZ);
        }
    }

    void clearFightDeployment()
    {
        fightDeploymentActive = false;
        fightDeploymentTime = 0.0f;
        unitDeployActive.fill(false);
        enemyDeployActive.fill(false);
    }

    void beginMovingCombatantDeployment(CountMastersEnemySquad &enemy)
    {
        std::array<glm::vec2, MAX_UNITS> playerTargets{};
        std::array<glm::vec2, 64> enemyTargets{};
        assignSharedFightSlots(enemy, playerTargets, enemyTargets);

        clearFightDeployment();
        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Moving)
                continue;
            unitDeployActive[p] = true;
            unitDeployStart[p] = units[p];
            unitDeployTarget[p] = playerTargets[p];
            fightDeploymentActive = true;
        }
        for (int e = 0; e < enemy.count; ++e)
        {
            if (enemy.modes[e] != CountMastersUnitMode::Moving)
                continue;
            enemyDeployActive[e] = true;
            enemyDeployStart[e] = enemy.units[e];
            enemyDeployTarget[e] = enemyTargets[e];
            fightDeploymentActive = true;
        }
    }

    void updateFightDeployment(CountMastersEnemySquad &enemy, float dt)
    {
        if (!fightDeploymentActive)
            return;

        fightDeploymentTime += dt;
        const float t = std::clamp(fightDeploymentTime / FIGHT_DEPLOY_DURATION_S, 0.0f, 1.0f);
        const float ease = t * t * (3.0f - 2.0f * t);
        for (int p = 0; p < playerCount; ++p)
        {
            if (!unitDeployActive[p])
                continue;
            units[p] = unitDeployStart[p] + (unitDeployTarget[p] - unitDeployStart[p]) * ease;
        }
        for (int e = 0; e < enemy.count; ++e)
        {
            if (!enemyDeployActive[e])
                continue;
            enemy.units[e] = enemyDeployStart[e] + (enemyDeployTarget[e] - enemyDeployStart[e]) * ease;
        }

        if (t >= 1.0f)
            clearFightDeployment();
    }

    void enforceFightSeparation(CountMastersEnemySquad &enemy)
    {
        // Fighting pairs are locked in place; nearby movers yield so only active pairs blink/fight.
        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] == CountMastersUnitMode::Dead)
                continue;
            for (int e = 0; e < enemy.count; ++e)
            {
                if (enemy.modes[e] == CountMastersUnitMode::Dead)
                    continue;
                if (unitModes[p] == CountMastersUnitMode::Fighting &&
                    enemy.modes[e] == CountMastersUnitMode::Fighting &&
                    unitTargetEnemy[p] == e)
                {
                    continue;
                }
                if (unitModes[p] == CountMastersUnitMode::Fighting &&
                    enemy.modes[e] == CountMastersUnitMode::Moving)
                {
                    PushApart(units[p], enemy.units[e], FORMATION_MIN_SEPARATION_M, 0.0f, 1.0f);
                }
                else if (unitModes[p] == CountMastersUnitMode::Moving &&
                         enemy.modes[e] == CountMastersUnitMode::Fighting)
                {
                    if (p != 0)
                        PushApart(units[p], enemy.units[e], FORMATION_MIN_SEPARATION_M, 1.0f, 0.0f);
                }
            }
        }
        for (int f = 0; f < playerCount; ++f)
        {
            if (unitModes[f] != CountMastersUnitMode::Fighting)
                continue;
            for (int m = 0; m < playerCount; ++m)
            {
                if (unitModes[m] == CountMastersUnitMode::Moving)
                    PushApart(units[f], units[m], FORMATION_MIN_SEPARATION_M, 0.0f, 1.0f);
            }
        }
        for (int f = 0; f < enemy.count; ++f)
        {
            if (enemy.modes[f] != CountMastersUnitMode::Fighting)
                continue;
            for (int m = 0; m < enemy.count; ++m)
            {
                if (enemy.modes[m] == CountMastersUnitMode::Moving)
                    PushApart(enemy.units[f], enemy.units[m], FORMATION_MIN_SEPARATION_M, 0.0f, 1.0f);
            }
        }

        for (int p = 1; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Moving)
                continue;
            PushApart(units[0], units[p], FORMATION_MIN_SEPARATION_M, 0.0f, 1.0f);
        }
        for (int a = 1; a < playerCount; ++a)
        {
            if (unitModes[a] != CountMastersUnitMode::Moving)
                continue;
            for (int b = a + 1; b < playerCount; ++b)
            {
                if (unitModes[b] == CountMastersUnitMode::Moving)
                    PushApart(units[a], units[b], FORMATION_MIN_SEPARATION_M);
            }
        }
        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Moving)
                continue;
            for (int e = 0; e < enemy.count; ++e)
            {
                if (enemy.modes[e] != CountMastersUnitMode::Moving)
                    continue;
                const float playerWeight = (p == 0) ? 0.0f : 0.5f;
                const float enemyWeight = (p == 0) ? 1.0f : 0.5f;
                PushApart(units[p], enemy.units[e], FORMATION_MIN_SEPARATION_M, playerWeight, enemyWeight);
            }
        }
        for (int a = 0; a < enemy.count; ++a)
        {
            if (enemy.modes[a] != CountMastersUnitMode::Moving)
                continue;
            for (int b = a + 1; b < enemy.count; ++b)
            {
                if (enemy.modes[b] == CountMastersUnitMode::Moving)
                    PushApart(enemy.units[a], enemy.units[b], FORMATION_MIN_SEPARATION_M);
            }
        }
    }

    void beginFightPair(CountMastersEnemySquad &enemy, int playerIndex, int enemyIndex)
    {
        if (playerIndex < 0 || playerIndex >= playerCount || enemyIndex < 0 || enemyIndex >= enemy.count)
            return;
        if (unitModes[playerIndex] != CountMastersUnitMode::Moving ||
            enemy.modes[enemyIndex] != CountMastersUnitMode::Moving)
        {
            return;
        }

        const glm::vec2 midpoint = (units[playerIndex] + enemy.units[enemyIndex]) * 0.5f;
        glm::vec2 separation = enemy.units[enemyIndex] - units[playerIndex];
        if (separation.x * separation.x + separation.y * separation.y < 0.000001f)
            separation = glm::vec2(1.0f, 0.0f);
        separation = glm::normalize(separation) * (FORMATION_MIN_SEPARATION_M * 0.5f);
        units[playerIndex] = midpoint - separation;
        enemy.units[enemyIndex] = midpoint + separation;
        unitModes[playerIndex] = CountMastersUnitMode::Fighting;
        enemy.modes[enemyIndex] = CountMastersUnitMode::Fighting;
        unitTargetEnemy[playerIndex] = enemyIndex;
        enemy.targetPlayer[enemyIndex] = playerIndex;
        unitFightTime[playerIndex] = FIGHT_DURATION_S;
        enemy.fightTime[enemyIndex] = FIGHT_DURATION_S;
        sfxEvents.push(MiniGameSfxEvent::FIGHT_START);
        spawnParticleEvent(
            MiniGameParticleEventKind::FIGHT_CONTACT,
            midpoint,
            enemy.units[enemyIndex] - units[playerIndex],
            0.55f
        );
    }

    void tryBeginFightForPlayer(CountMastersEnemySquad &enemy, int playerIndex, std::array<bool, 64> &claimedEnemies)
    {
        if (playerIndex < 0 || playerIndex >= playerCount || unitModes[playerIndex] != CountMastersUnitMode::Moving)
            return;

        const int targetEnemy = nearestUntackledEnemyIndex(enemy, units[playerIndex], claimedEnemies);
        if (targetEnemy < 0)
            return;

        const glm::vec2 d = units[playerIndex] - enemy.units[targetEnemy];
        if (d.x * d.x + d.y * d.y > FIGHT_TRIGGER_DISTANCE_M * FIGHT_TRIGGER_DISTANCE_M)
            return;

        claimedEnemies[targetEnemy] = true;
        beginFightPair(enemy, playerIndex, targetEnemy);
    }

    void beginAvailableFightPairs(CountMastersEnemySquad &enemy, bool allowLeaderWithFollowers)
    {
        std::array<bool, 64> claimedEnemies{};
        for (int i = 0; i < enemy.count; ++i)
            claimedEnemies[i] = enemy.modes[i] != CountMastersUnitMode::Moving;

        for (int p = 1; p < playerCount; ++p)
            tryBeginFightForPlayer(enemy, p, claimedEnemies);

        if (allowLeaderWithFollowers)
        {
            tryBeginFightForPlayer(enemy, 0, claimedEnemies);
        }
        else
        {
            int movingPlayers = 0;
            for (int p = 0; p < playerCount; ++p)
                movingPlayers += unitModes[p] == CountMastersUnitMode::Moving ? 1 : 0;
            if (movingPlayers <= 1)
                tryBeginFightForPlayer(enemy, 0, claimedEnemies);
        }
    }

    void moveRestingCombatantsTowardContact(CountMastersEnemySquad &enemy, float dt)
    {
        const float step = FIGHTER_SPEED_MPS * dt;
        const float playerMaxY = runnerZ - FORMATION_MIN_SEPARATION_M * 0.5f;
        const float enemyMinY = runnerZ + FORMATION_MIN_SEPARATION_M * 0.5f;
        int movingFollowers = 0;
        for (int p = 1; p < playerCount; ++p)
            movingFollowers += unitModes[p] == CountMastersUnitMode::Moving ? 1 : 0;
        const int firstMovingPlayer = movingFollowers > 0 ? 1 : 0;

        for (int p = firstMovingPlayer; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Moving)
                continue;
            const int e = nearestUntackledEnemyIndex(enemy, units[p], {});
            if (e < 0)
                continue;
            units[p] = MovePointTowards(units[p], enemy.units[e], step);
            units[p].y = std::min(units[p].y, playerMaxY);
            units[p].x = std::clamp(units[p].x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        }

        for (int e = 0; e < enemy.count; ++e)
        {
            if (enemy.modes[e] != CountMastersUnitMode::Moving)
                continue;

            int bestPlayer = -1;
            float bestDist2 = 1.0e9f;
            for (int p = firstMovingPlayer; p < playerCount; ++p)
            {
                if (unitModes[p] != CountMastersUnitMode::Moving)
                    continue;
                const glm::vec2 d = units[p] - enemy.units[e];
                const float dist2 = d.x * d.x + d.y * d.y;
                if (dist2 < bestDist2)
                {
                    bestDist2 = dist2;
                    bestPlayer = p;
                }
            }
            if (bestPlayer < 0)
                continue;

            enemy.units[e] = MovePointTowards(enemy.units[e], units[bestPlayer], step);
            enemy.units[e].y = std::max(enemy.units[e].y, enemyMinY);
            enemy.units[e].x = std::clamp(enemy.units[e].x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        }
    }

    void startEnemyEngagement(int squadIndex, glm::vec2 contactCenter)
    {
        if (squadIndex < 0 || squadIndex >= ENEMY_SQUAD_COUNT)
            return;
        CountMastersEnemySquad &enemy = enemies[squadIndex];
        if (enemy.resolved || enemy.count <= 0)
            return;
        enemy.engaged = true;
        enemy.leaderReachedCenter = true;
        runnerX = std::clamp(contactCenter.x, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        runnerZ = contactCenter.y;
        activeFightSquad = squadIndex;
        beginMovingCombatantDeployment(enemy);
    }

    void updateFight(float dt)
    {
        if (activeFightSquad < 0 || activeFightSquad >= ENEMY_SQUAD_COUNT)
            return;

        CountMastersEnemySquad &enemy = enemies[activeFightSquad];
        electMovingLeaderIfNeeded();
        enemy.engaged = true;
        enemy.leaderReachedCenter = true;
        targetX = runnerX;

        if (fightDeploymentActive)
        {
            updateFightDeployment(enemy, dt);
            if (playerCount > 0 && unitModes[0] == CountMastersUnitMode::Moving)
            {
                runnerX = units[0].x;
                runnerZ = units[0].y;
                targetX = runnerX;
            }
            if (fightDeploymentActive)
                return;
        }

        if (playerCount > 0 && unitModes[0] == CountMastersUnitMode::Moving)
            units[0] = glm::vec2(runnerX, runnerZ);

        moveRestingCombatantsTowardContact(enemy, dt);
        beginAvailableFightPairs(enemy, false);

        enforceFightSeparation(enemy);

        electMovingLeaderIfNeeded();
        if (playerCount > 0 && unitModes[0] == CountMastersUnitMode::Moving)
        {
            runnerX = units[0].x;
            runnerZ = units[0].y;
        }

        for (int p = 0; p < playerCount; ++p)
        {
            if (unitModes[p] != CountMastersUnitMode::Fighting)
                continue;
            unitFightTime[p] -= dt;
            if (unitFightTime[p] <= 0.0f)
            {
                const int e = unitTargetEnemy[p];
                spawnDeathFx(units[p], true, glm::vec2(0.0f, RUN_SPEED_MPS));
                if (e >= 0 && e < enemy.count)
                    spawnDeathFx(enemy.units[e], false, glm::vec2(0.0f, -RUN_SPEED_MPS));
                unitModes[p] = CountMastersUnitMode::Dead;
                if (e >= 0 && e < enemy.count)
                    enemy.modes[e] = CountMastersUnitMode::Dead;
            }
        }

        const int playersLeft = activePlayerCount();
        const int enemiesLeft = activeEnemyCount(enemy);
        if (playersLeft <= 0)
        {
            compactLivePlayers();
            phase = CountMastersPhase::LOST;
            return;
        }
        if (enemiesLeft <= 0)
        {
            compactLivePlayers();
            compactEnemySquad(enemy);
            enemy.resolved = true;
            enemy.engaged = false;
            activeFightSquad = -1;
            return;
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
        standers = 0;
        rewardCoins = 0;
        gateCoinsCollected = 0;
        activeFightSquad = -1;
        waitingForFirstInput = false;
        clearFightDeployment();
        resetPins();
        pinCrashCenter = glm::vec2(0.0f);
        pinCrashVelocity = glm::vec2(0.0f, RUN_SPEED_MPS);
        pinCrashNeedsPhysicsStart = false;
        pinCrashPhysicsStarted = false;
        pinCrashScoringComplete = false;
        pinCrashResultHoldTime = 0.0f;
        motionHistoryPos.fill(glm::vec2(0.0f));
        motionHistoryTime.fill(0.0f);
        motionHistoryWrite = 0;
        motionHistoryUsed = 0;
        deathFx.fill({});
        sfxEvents.clear();
        particleEvents.clear();
        deathFxCursor = 0;
        deathFxSeed = 0xA341316Cu;
        units.fill(glm::vec2(0.0f));
        unitModes.fill(CountMastersUnitMode::Dead);
        unitTargetEnemy.fill(-1);
        unitFightTime.fill(0.0f);
        units[0] = glm::vec2(runnerX, runnerZ);
        unitModes[0] = CountMastersUnitMode::Moving;
        gateShards.fill({});

        gates = {{
            {-13.10f, {CountMastersOp::ADD, 5}, {CountMastersOp::MULTIPLY, 3}, false},
            {-10.15f, {CountMastersOp::MULTIPLY, 2}, {CountMastersOp::ADD, 12}, false},
            {-7.35f, {CountMastersOp::SUBTRACT, 4}, {CountMastersOp::MULTIPLY, 5}, false},
            {-4.55f, {CountMastersOp::DIVIDE, 2}, {CountMastersOp::ADD, 20}, false},
        }};
        for (int gateIndex = 0; gateIndex < GATE_COUNT; ++gateIndex)
        {
            gates[gateIndex].leftCoinCount = 1 + ((gateIndex * 2) % MAX_GATE_COINS_PER_SIDE);
            gates[gateIndex].rightCoinCount = 1 + ((gateIndex * 2 + 3) % MAX_GATE_COINS_PER_SIDE);
        }
        enemies = {{
            {-11.90f, 2, false},
            {-9.00f, 9, false},
            {-6.20f, 13, false},
            {-3.15f, 18, false},
        }};
        for (CountMastersEnemySquad &enemy : enemies)
            InitEnemySquadUnits(enemy);
    }

    void initWithSeed(uint32_t seed)
    {
        initDefault();
        if (seed == 0u)
            seed = 1u;
        gates = {{
            MakeVariedGateRow(0, -13.10f, seed),
            MakeVariedGateRow(1, -10.15f, seed),
            MakeVariedGateRow(2, -7.35f, seed),
            MakeVariedGateRow(3, -4.55f, seed),
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
        updateDeathFx(dt);
        if (phase != CountMastersPhase::RUNNING && phase != CountMastersPhase::PIN_CRASH)
            return;

        dt = std::clamp(dt, 0.0f, 0.05f);
        if (waitingForFirstInput)
        {
            targetX = std::clamp(inputX, -LANE_HALF_WIDTH, LANE_HALF_WIDTH);
            return;
        }
        elapsed += dt;

        if (phase == CountMastersPhase::PIN_CRASH)
        {
            updateGateShards(dt);
            return;
        }

        if (activeFightSquad >= 0)
        {
            updateFight(dt);
            updateGateShards(dt);
            return;
        }

        targetX = std::clamp(inputX, -LANE_HALF_WIDTH, LANE_HALF_WIDTH);
        const float follow = std::clamp(dt * SIDE_FOLLOW_SPEED, 0.0f, 1.0f);
        runnerX += (targetX - runnerX) * follow;
        runnerX = std::clamp(runnerX, -LANE_HALF_WIDTH + 0.03f, LANE_HALF_WIDTH - 0.03f);
        runnerZ += RUN_SPEED_MPS * dt;
        units[0] = glm::vec2(runnerX, runnerZ);

        for (int gateIndex = 0; gateIndex < GATE_COUNT; ++gateIndex)
        {
            CountMastersGateRow &gate = gates[gateIndex];
            if (gate.resolved || runnerZ < gate.z)
                continue;
            gate.resolved = true;
            gate.chosenSide = (runnerX < 0.0f) ? -1 : 1;
            spawnGateShards(gateIndex, gate.chosenSide);
            gateCoinsCollected += gate.chosenSide < 0 ? gate.leftCoinCount : gate.rightCoinCount;
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

        for (int enemyIndex = 0; enemyIndex < ENEMY_SQUAD_COUNT; ++enemyIndex)
        {
            CountMastersEnemySquad &enemy = enemies[enemyIndex];
            if (enemy.resolved)
                continue;
            glm::vec2 contactCenter(0.0f);
            if (firstContactWithEnemy(enemy, &contactCenter))
            {
                startEnemyEngagement(enemyIndex, contactCenter);
                updateFight(dt);
                updateGateShards(dt);
                return;
            }
        }

        updateFollowerUnits(dt);
        updateGateShards(dt);
        recordMotionHistory();

        if (anyMovingPlayerTouchesStandingPin())
        {
            beginPinCrash();
            return;
        }

        if (runnerZ > PIN_RACK_BACK_Z + 0.70f)
        {
            standers = 0;
            rewardCoins = 0;
            phase = CountMastersPhase::LOST;
        }
    }
};
