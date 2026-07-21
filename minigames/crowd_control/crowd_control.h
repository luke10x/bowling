#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <glm/glm.hpp>

#include "../minigame_sfx_events.h"

enum class CrowdControlPhase : uint8_t
{
    INACTIVE = 0,
    RUNNING = 1,
    WON = 2,
    LOST = 3,
};

enum class CrowdControlEnemyKind : uint8_t
{
    DOG = 0,
    SERAPH = 1,
    THRONE = 2,
};

enum class CrowdControlCardKind : uint8_t
{
    RATE = 0,
    POWER = 1,
};

enum class CrowdControlUnitMode : uint8_t
{
    MOVING = 0,
    FIGHTING = 1,
};

enum class CrowdControlUnitLane : uint8_t
{
    COMBAT = 0,
    LEFT_REWARD = 1,
    RIGHT_REWARD = 2,
};

enum class CrowdControlEndReason : uint8_t
{
    NONE = 0,
    MALACH_REACHED_ENEMY_BASE = 1,
    ENEMY_REACHED_SPAWN = 2,
};

struct CrowdControlTuning
{
    float spawnMargin = 0.50f;
    float unitRadius = 0.08f;
    float unitSpeed = 1.50f;
    float brownSpeed = 0.25f;
    float leftUpgradeSpeed = 0.60f;
    float leftUpgradeStep = 0.15f;
    int leftUpgradePrice = 1;
    float rightUpgradeSpeed = 0.08f;
    float rightUpgradeStep = 2.50f;
    int rightUpgradePrice = 99;
    int upgradeBeltSpeedBoostEnemySpawnPeriod = 100;
    float upgradeBeltSpeedBoostMultiplier = 1.1f;
    float missedRewardExitTailDistance = 0.20f;
    float noSpawnIfCloserThan = 2.0f;
    float addedEnemyDelay = 5.0f;
    float ourStartingTtl = 2.0f; // we start with half ttl
    float enemyStartingTtl = 4.0f;
    float ourStartingHitBuff = 1.0f;
    int earlyAngelDamageBoostSpawnCount = 50;
    float earlyAngelDamageBoostMultiplier = 2.0f;
    float enemyStartingHitBuff = 1.0f;
    float enemyStartingHealthBuff = 1.0f;
    float angelSpeedUpgradeMultiplier = 1.35f;
    float angelSpawnRateUpgradeMultiplier = 1.5f;
    float angelSpawnRateBoostDecayPerSecond = 0.50f;
    float angelTtlUpgradeMultiplier = 1.5f;
    float angelHitBuffUpgradeMultiplier = 1.75f;
    float enemySpawnDamageMultiplier = 1.002f;
    float enemySpawnHealthMultiplier = 1.002f;
    float ourSpawnRate = 2.0f;
    float enemySpawnRate = 1.25f;
    float enemySideFrontlineSpawnMultiplier = 1.5f;
    float enemySideFrontlineThreshold01 = 0.5f;
    float chanceToTurnAroundPerSecond = 0.5f;
    float blockedDamagePerSecond = 0.1f;
    float graceTime = 0.8f;
    float controlSpeed = 0.8f;
    float inputFollowSpeed = 9.0f;
    float bossScaleTtl = 1.0f;
    float bossSmashRadius = 0.42f;
    float seraphSmashDamage = 2.0f;
    float throneSmashDamage = 3.5f;
    int seraphSmashMaxTargets = 3;
    int throneSmashMaxTargets = 6;
};

// Hot-reloadable gameplay tuning. Keep memory sizes in CrowdControlState fixed,
// but edit values here while the game is running to reshape the minigame safely.
static inline CrowdControlTuning CrowdControl_GetTuning()
{
    return CrowdControlTuning{};
}

struct CrowdControlWaveSegment
{
    CrowdControlEnemyKind kind;
    int count;
};

struct CrowdControlUnit
{
    bool active = false;
    glm::vec2 pos = glm::vec2(0.0f); // x,z in lane-space
    glm::vec2 lastMove = glm::vec2(0.0f, 1.0f);
    int hp = 1;
    int maxHp = 1;
    float fightStrength = 0.0f; // JS ttl
    float maxFightStrength = 0.0f;
    CrowdControlEnemyKind kind = CrowdControlEnemyKind::DOG;
    CrowdControlUnitMode mode = CrowdControlUnitMode::MOVING;
    float fightTime = 0.0f; // used for blink animation; counts up while fighting
    int pairedIndex = -1;
    int sidestepDir = 1; // JS brown
    uint32_t spawnSeq = 0;
    CrowdControlUnitLane lane = CrowdControlUnitLane::COMBAT;
    float meleeCooldown = 0.0f;
    bool blocked = false;
    bool bossBlocked = false;
    bool canFight = true;
    bool skipFirstMove = false;
    float graceTime = 0.0f;
    float speed = 0.0f;
    float hitBuff = 1.0f;
};

struct CrowdControlDeathFx
{
    bool active = false;
    bool malach = false;
    CrowdControlEnemyKind kind = CrowdControlEnemyKind::DOG;
    glm::vec2 startPos = glm::vec2(0.0f);
    glm::vec2 flyDir = glm::vec2(1.0f, 0.0f);
    float age = 0.0f;
    float duration = 0.55f;
    float distance = 1.2f;
    float arcHeight = 0.45f;
    float spin = 0.0f;
    float sideSign = 1.0f;
};

struct CrowdControlCard
{
    bool active = false;
    bool pickable = true;
    CrowdControlCardKind kind = CrowdControlCardKind::RATE;
    glm::vec2 pos = glm::vec2(0.0f);
    int hp = 1;
    int maxHp = 1;
};

struct CrowdControlCardLabel
{
    bool active = false;
    int cardIndex = -1;
    char text[8] = {};
};

struct CrowdControlFloatingText
{
    bool active = false;
    bool consumed = false;
    glm::vec2 cardPos = glm::vec2(0.0f);
    char text[8] = {};
    float age = 0.0f;
    float duration = 0.85f;
};

struct CrowdControlBossHpText
{
    bool active = false;
    glm::vec2 enemyPos = glm::vec2(0.0f);
    CrowdControlEnemyKind kind = CrowdControlEnemyKind::SERAPH;
    char text[8] = {};
    float age = 0.0f;
    float duration = 0.85f;
};

struct CrowdControlState
{
    // Memory doctrine: these are preallocated buffers stored on usr through this
    // state object. Do not allocate per-frame or hide gameplay buffers in globals.
    static inline constexpr int MAX_MALACHIM = 1000;
    static inline constexpr int MAX_ENEMIES = 1000;
    static inline constexpr int MAX_CARDS = 192;
    static inline constexpr int MAX_MISSED_CARDS = 32;
    static inline constexpr int MAX_DEATH_FX = 256;
    static inline constexpr int MAX_FLOATING_TEXTS = 64;
    static inline constexpr int MAX_BOSS_HP_TEXTS = 64;
    static inline constexpr int MAX_SFX_EVENTS = 64;
    static inline constexpr int MAX_PARTICLE_EVENTS = 96;
    static inline constexpr int OBSERVED_MALACH_SPAWN_SAMPLES = 5;
    static inline constexpr float LANE_HALF_WIDTH = 1.07f * 0.5f;
    static inline constexpr float LANE_WIDTH = LANE_HALF_WIDTH * 2.0f;
    static inline constexpr float SIDE_STRIP_WIDTH = LANE_WIDTH / 6.0f;
    static inline constexpr float MIDDLE_HALF_WIDTH = LANE_HALF_WIDTH - SIDE_STRIP_WIDTH;
    static inline constexpr float LANE_SEAM_SNAP_MARGIN = LANE_WIDTH / 12.0f;
    static inline constexpr float SIDE_CORRIDOR_INSET = 0.025f;
    static inline constexpr float LANE_START_Z = -18.0f;
    static inline constexpr float LANE_END_Z = LANE_START_Z + 18.27f;
    static inline constexpr float LANE_LENGTH = LANE_END_Z - LANE_START_Z;
    static inline constexpr float LANE_VISIBLE_START_Z = -18.30f;
    static inline constexpr float LANE_VISIBLE_LENGTH = LANE_END_Z - LANE_VISIBLE_START_Z;
    static inline constexpr int CARD_LABEL_SLOTS = MAX_CARDS;
    static inline constexpr float MALACH_BASE_FIGHT_STRENGTH_S = 0.50f;
    static inline constexpr float DOG_FIGHT_STRENGTH_S = 0.75f;
    static inline constexpr int SERAPH_HP = 200;
    static inline constexpr int THRONE_HP = 500;
    static inline constexpr float SERAPH_MELEE_COOLDOWN_S = 1.20f;
    static inline constexpr float THRONE_MELEE_COOLDOWN_S = 0.90f;
    static inline constexpr int MAX_BOSS_SMASH_TARGETS = 8;
    static inline constexpr int STARTING_FORTRESS_HP = 8;

    static inline constexpr CrowdControlWaveSegment DEFAULT_STREAM[] = {
        {CrowdControlEnemyKind::DOG, 80},
        {CrowdControlEnemyKind::SERAPH, 1},
        {CrowdControlEnemyKind::DOG, 50},
        {CrowdControlEnemyKind::SERAPH, 1},
        {CrowdControlEnemyKind::DOG, 30},
        {CrowdControlEnemyKind::SERAPH, 1},
        {CrowdControlEnemyKind::DOG, 20},
        {CrowdControlEnemyKind::SERAPH, 2},
        {CrowdControlEnemyKind::DOG, 40},
        {CrowdControlEnemyKind::SERAPH, 2},
        {CrowdControlEnemyKind::DOG, 30},
        {CrowdControlEnemyKind::SERAPH, 2},
        {CrowdControlEnemyKind::DOG, 20},
        {CrowdControlEnemyKind::THRONE, 1},
        {CrowdControlEnemyKind::DOG, 20},
    };
    static inline constexpr int DEFAULT_STREAM_COUNT = (int)(sizeof(DEFAULT_STREAM) / sizeof(DEFAULT_STREAM[0]));

    CrowdControlPhase phase = CrowdControlPhase::INACTIVE;
    CrowdControlEndReason endReason = CrowdControlEndReason::NONE;
    bool waitingForFirstInput = false;
    float elapsed = 0.0f;
    float spawnX = 0.0f;
    float targetX = 0.0f;
    float fireAccumulator = 0.0f;
    float enemySpawnAccumulator = 0.0f;
    int rateUpgrade = 0;
    int malachHealthUpgrade = 0;
    int fortressHp = STARTING_FORTRESS_HP;
    int dogsKilled = 0;
    int bossesKilled = 0;
    int totalMalachimSpawned = 0;
    int totalEnemiesSpawned = 0;
    int bossHpRewardEarned = 0;
    int rewardCoins = 0;
    int waveIndex = 0;
    int waveRemaining = 0;
    int malachSpawnCursor = 0;
    int enemySpawnCursor = 0;
    uint32_t nextSpawnSeq = 1;
    int missedCardCursor = 0;
    int deathFxCursor = 0;
    int floatingTextCursor = 0;
    int bossHpTextCursor = 0;
    int observedMalachSpawnCursor = 0;
    int observedMalachSpawnCount = 0;
    uint32_t sideRng = 0x6D2B79F5u;
    bool spawnCubeFastBlinkOn = false;
    bool waveComplete = false;
    float staleNoKillTimer = 0.0f;
    int staleLastDestroyedCount = 0;
    float staleLastEnemyStrength = 0.0f;
    bool staleDumpedThisEpisode = false;

    // JS ctx fields, kept as data so hot reload updates behavior without moving memory.
    float lfo = 0.0f;
    float enemySpawnTimer = 0.0f;
    float ourSpawnTimer = 0.0f;
    bool enemyStarted = false;
    bool weStarted = false;
    float mySpawnRate = 2.0f;
    float theirSpawnRate = 1.0f;
    float mySpeed = 1.50f;
    // Runtime state: initCrowdControl() overwrites this from CrowdControlTuning::ourStartingTtl.
    float myTtl = 2.0f;
    float themTtl = 4.5f;
    float myHitBuff = 1.0f;
    float themHitBuff = 1.0f;
    float themHealthBuff = 1.0f;
    float enemyDelay = 5.0f;
    float leftBeltLen = 1.0f;
    float rightBeltLen = 15.0f;
    int leftBeltVal = 1;
    int rightBeltVal = 99;
    int myBrown = 1;
    int themBrown = 1;
    float frontline = LANE_LENGTH * 0.5f;

    std::array<CrowdControlUnit, MAX_MALACHIM> malachim{};
    std::array<CrowdControlUnit, MAX_ENEMIES> enemies{};
    std::array<CrowdControlCard, MAX_CARDS> cards{};
    std::array<CrowdControlCard, MAX_MISSED_CARDS> missedCards{};
    std::array<CrowdControlDeathFx, MAX_DEATH_FX> deathFx{};
    std::array<CrowdControlFloatingText, MAX_FLOATING_TEXTS> floatingTexts{};
    std::array<CrowdControlBossHpText, MAX_BOSS_HP_TEXTS> bossHpTexts{};
    std::array<float, OBSERVED_MALACH_SPAWN_SAMPLES> observedMalachSpawnTimes{};
    MiniGameSfxEventQueue<MAX_SFX_EVENTS> sfxEvents{};
    MiniGameParticleEventQueue<MAX_PARTICLE_EVENTS> particleEvents{};

    static inline bool IsBoss(CrowdControlEnemyKind kind)
    {
        return kind == CrowdControlEnemyKind::SERAPH || kind == CrowdControlEnemyKind::THRONE;
    }

    static inline int EnemyMaxHp(CrowdControlEnemyKind kind)
    {
        switch (kind)
        {
            case CrowdControlEnemyKind::SERAPH: return SERAPH_HP;
            case CrowdControlEnemyKind::THRONE: return THRONE_HP;
            case CrowdControlEnemyKind::DOG:
            default: return 1;
        }
    }

    static inline float EnemyTtlForKind(CrowdControlEnemyKind kind, const CrowdControlTuning &tuning)
    {
        if (kind == CrowdControlEnemyKind::SERAPH)
            return (float)SERAPH_HP * MALACH_BASE_FIGHT_STRENGTH_S * tuning.bossScaleTtl;
        if (kind == CrowdControlEnemyKind::THRONE)
            return (float)THRONE_HP * MALACH_BASE_FIGHT_STRENGTH_S * tuning.bossScaleTtl;
        return tuning.enemyStartingTtl;
    }

    static inline float SideCorridorCenter(float sign)
    {
        // This sign is world/lane X, not screen side. Crowd Control camera looks
        // from the negative-Z side, so screen-left is positive lane X.
        return sign < 0.0f
            ? -LANE_HALF_WIDTH + SIDE_STRIP_WIDTH * 0.5f
            : LANE_HALF_WIDTH - SIDE_STRIP_WIDTH * 0.5f;
    }

    static inline float ScreenLeftCorridorCenter()
    {
        return SideCorridorCenter(1.0f);
    }

    static inline float ScreenRightCorridorCenter()
    {
        return SideCorridorCenter(-1.0f);
    }

    static inline float CombatCenterLimit(const CrowdControlTuning &tuning)
    {
        (void)tuning;
        return LANE_WIDTH * 0.25f;
    }

    static inline float SnapLaneControlX(float x)
    {
        // Port of JS getSnappedSpawnX(), converted from [0..laneWidth] to centered lane X.
        const float jsX = std::clamp(x + LANE_HALF_WIDTH, 0.0f, LANE_WIDTH);
        if (jsX < LANE_WIDTH * 2.0f / 12.0f)
            return -LANE_HALF_WIDTH + LANE_WIDTH * 1.0f / 12.0f;
        if (jsX > LANE_WIDTH * 10.0f / 12.0f)
            return -LANE_HALF_WIDTH + LANE_WIDTH * 11.0f / 12.0f;
        if (jsX < LANE_WIDTH * 3.0f / 12.0f)
            return -LANE_HALF_WIDTH + LANE_WIDTH * 3.0f / 12.0f;
        if (jsX > LANE_WIDTH * 9.0f / 12.0f)
            return -LANE_HALF_WIDTH + LANE_WIDTH * 9.0f / 12.0f;
        return jsX - LANE_HALF_WIDTH;
    }

    static inline float ClampReleaseXToSnappedCorridor(float snappedBaseX, float x)
    {
        if (snappedBaseX <= -MIDDLE_HALF_WIDTH)
            return SideCorridorCenter(-1.0f);
        if (snappedBaseX >= MIDDLE_HALF_WIDTH)
            return SideCorridorCenter(1.0f);
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float combatCenterLimit = CombatCenterLimit(tuning);
        return std::clamp(x, -combatCenterLimit, combatCenterLimit);
    }

    static inline float PortraitMappedX01(
        float logicalScreenX,
        float logicalWidth,
        float logicalHeight
    )
    {
        const float w = std::max(1.0f, logicalWidth);
        const float h = std::max(1.0f, logicalHeight);
        constexpr float portraitAspect = 9.0f / 16.0f;
        const float portraitWidth = (w / h) > portraitAspect ? h * portraitAspect : w;
        const float portraitLeft = (w - portraitWidth) * 0.5f;
        return std::clamp((logicalScreenX - portraitLeft) / std::max(1.0f, portraitWidth), 0.0f, 1.0f);
    }

    static inline float ScreenXToLaneX(
        float logicalScreenX,
        float logicalWidth,
        float logicalHeight,
        float laneHalfWidth
    )
    {
        const float x01 = PortraitMappedX01(logicalScreenX, logicalWidth, logicalHeight);
        return (0.5f - x01) * 2.0f * laneHalfWidth;
    }

    static inline float jsZFromWorld(float worldZ)
    {
        return LANE_END_Z - worldZ;
    }

    static inline float worldZFromJs(float jsZ)
    {
        return LANE_END_Z - jsZ;
    }

    static inline int HpFromFightStrength(float strength)
    {
        return std::max(0, (int)std::ceil(strength / MALACH_BASE_FIGHT_STRENGTH_S));
    }

    static inline float CardVisualDepth(CrowdControlCardKind kind)
    {
        return kind == CrowdControlCardKind::RATE ? 0.035f : 0.050f;
    }

    static inline float CardVisualHalfDepth(CrowdControlCardKind kind)
    {
        return CardVisualDepth(kind) * 0.5f;
    }

    static inline float MissedRewardLaneEndJsZ(CrowdControlCardKind kind)
    {
        return LANE_VISIBLE_LENGTH + CardVisualHalfDepth(kind);
    }

    static inline float MissedRewardExitJsZ(CrowdControlCardKind kind)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        return MissedRewardLaneEndJsZ(kind) + tuning.missedRewardExitTailDistance;
    }

    static inline bool MissedRewardShouldRender(CrowdControlCardKind kind, float jsZ)
    {
        const float exitJsZ = MissedRewardExitJsZ(kind);
        return jsZ < exitJsZ;
    }

    static inline float MissedRewardAlpha(CrowdControlCardKind kind, float jsZ)
    {
        const float laneEndJsZ = MissedRewardLaneEndJsZ(kind);
        if (jsZ < laneEndJsZ)
            return 1.0f;
        const float exitJsZ = MissedRewardExitJsZ(kind);
        const float tail = std::max(0.001f, exitJsZ - laneEndJsZ);
        return std::clamp(1.0f - (jsZ - laneEndJsZ) / tail, 0.0f, 1.0f);
    }

    uint32_t nextRand()
    {
        sideRng ^= sideRng << 13;
        sideRng ^= sideRng >> 17;
        sideRng ^= sideRng << 5;
        return sideRng;
    }

    float nextRand01()
    {
        return (float)(nextRand() & 0x00FFFFFFu) / (float)0x01000000u;
    }

    int nextSideDir()
    {
        return (nextRand() & 1u) ? 1 : -1;
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

    float shotsPerSecond() const
    {
        return mySpawnRate;
    }

    float spawnedMalachimPerSecond() const
    {
        return mySpawnRate;
    }

    float spawnedMalachimPerMinute() const
    {
        return mySpawnRate * 60.0f;
    }

    float observedMalachimPerMinute() const
    {
        if (observedMalachSpawnCount <= 1)
            return 0.0f;

        const int oldestIndex =
            (observedMalachSpawnCursor - observedMalachSpawnCount + OBSERVED_MALACH_SPAWN_SAMPLES) %
            OBSERVED_MALACH_SPAWN_SAMPLES;
        const float oldestTime = observedMalachSpawnTimes[oldestIndex];
        const float eventIntervals = (float)(observedMalachSpawnCount - 1);
        const float window = std::max(elapsed - oldestTime, 1.0e-3f);
        return eventIntervals * 60.0f / window;
    }

    float newMalachTtlSeconds() const
    {
        return myTtl;
    }

    float newMalachHitBuff() const
    {
        return myHitBuff;
    }

    int activeMalachCount() const
    {
        int count = 0;
        for (const CrowdControlUnit &m : malachim)
            count += m.active ? 1 : 0;
        return count;
    }

    int activeEnemyCount() const
    {
        int count = 0;
        for (const CrowdControlUnit &e : enemies)
            count += e.active ? 1 : 0;
        return count;
    }

    int destroyedEnemyCount() const
    {
        return dogsKilled + bossesKilled;
    }

    void initDefault()
    {
        initCrowdControl();
    }

    void initCrowdControl()
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        phase = CrowdControlPhase::RUNNING;
        endReason = CrowdControlEndReason::NONE;
        waitingForFirstInput = true;
        elapsed = 0.0f;
        spawnX = 0.0f;
        targetX = 0.0f;
        fireAccumulator = 0.0f;
        enemySpawnAccumulator = 0.0f;
        rateUpgrade = 0;
        malachHealthUpgrade = 0;
        fortressHp = STARTING_FORTRESS_HP;
        dogsKilled = 0;
        bossesKilled = 0;
        totalMalachimSpawned = 0;
        totalEnemiesSpawned = 0;
        bossHpRewardEarned = 0;
        rewardCoins = 0;
        waveIndex = 0;
        waveRemaining = DEFAULT_STREAM[0].count;
        malachSpawnCursor = 0;
        enemySpawnCursor = 0;
        nextSpawnSeq = 1;
        missedCardCursor = 0;
        deathFxCursor = 0;
        floatingTextCursor = 0;
        bossHpTextCursor = 0;
        observedMalachSpawnCursor = 0;
        observedMalachSpawnCount = 0;
        sideRng = 0x6D2B79F5u;
        spawnCubeFastBlinkOn = false;
        waveComplete = false;
        staleNoKillTimer = 0.0f;
        staleLastDestroyedCount = 0;
        staleLastEnemyStrength = 0.0f;
        staleDumpedThisEpisode = false;
        lfo = 0.0f;
        enemySpawnTimer = 0.0f;
        ourSpawnTimer = 0.0f;
        enemyStarted = false;
        weStarted = false;
        mySpawnRate = tuning.ourSpawnRate;
        theirSpawnRate = tuning.enemySpawnRate;
        mySpeed = tuning.unitSpeed;
        myTtl = tuning.ourStartingTtl;
        themTtl = tuning.enemyStartingTtl;
        myHitBuff = tuning.ourStartingHitBuff;
        themHitBuff = tuning.enemyStartingHitBuff;
        themHealthBuff = tuning.enemyStartingHealthBuff;
        enemyDelay = tuning.addedEnemyDelay;
        leftBeltLen = std::min(15.0f, LANE_LENGTH - tuning.spawnMargin);
        rightBeltLen = std::min(15.0f, LANE_LENGTH - tuning.spawnMargin);
        leftBeltVal = tuning.leftUpgradePrice;
        rightBeltVal = tuning.rightUpgradePrice;
        myBrown = 1;
        themBrown = 1;
        frontline = LANE_LENGTH * 0.5f;
        for (CrowdControlUnit &m : malachim)
            m = CrowdControlUnit{};
        for (CrowdControlUnit &e : enemies)
            e = CrowdControlUnit{};
        for (CrowdControlCard &card : missedCards)
            card = CrowdControlCard{};
        for (CrowdControlDeathFx &fx : deathFx)
            fx = CrowdControlDeathFx{};
        for (CrowdControlFloatingText &text : floatingTexts)
            text = CrowdControlFloatingText{};
        for (CrowdControlBossHpText &text : bossHpTexts)
            text = CrowdControlBossHpText{};
        sfxEvents.clear();
        particleEvents.clear();
        syncCardsFromBelts();
    }

    void recordObservedMalachSpawn()
    {
        observedMalachSpawnTimes[observedMalachSpawnCursor] = elapsed;
        observedMalachSpawnCursor = (observedMalachSpawnCursor + 1) % OBSERVED_MALACH_SPAWN_SAMPLES;
        observedMalachSpawnCount = std::min(observedMalachSpawnCount + 1, OBSERVED_MALACH_SPAWN_SAMPLES);
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

    void spawnUpgradeFeedback(CrowdControlCardKind kind, bool consumed, glm::vec2 pos, glm::vec2 dir)
    {
        const MiniGameParticleEventKind particleKind =
            consumed && kind == CrowdControlCardKind::POWER
                ? MiniGameParticleEventKind::POWER_UPGRADE_CONSUMED
                : (consumed ? MiniGameParticleEventKind::UPGRADE_CONSUMED : MiniGameParticleEventKind::UPGRADE_HIT);
        spawnParticleEvent(
            particleKind,
            pos,
            dir,
            consumed ? (kind == CrowdControlCardKind::POWER ? 1.35f : 1.0f) : 0.45f
        );
        if (consumed && kind == CrowdControlCardKind::POWER)
            sfxEvents.push(MiniGameSfxEvent::POWER_UPGRADE_CONSUMED);
    }

    void spawnFloatingUpgradeText(glm::vec2 cardPos, int value, bool consumed)
    {
        CrowdControlFloatingText &text = floatingTexts[floatingTextCursor];
        floatingTextCursor = (floatingTextCursor + 1) % MAX_FLOATING_TEXTS;
        text = CrowdControlFloatingText{};
        text.active = true;
        text.consumed = consumed;
        text.cardPos = cardPos;
        std::snprintf(text.text, sizeof(text.text), "%d", std::max(0, value));
    }

    void spawnBossHpText(const CrowdControlUnit &enemy, int hp)
    {
        if (!enemy.active || !IsBoss(enemy.kind))
            return;
        CrowdControlBossHpText &text = bossHpTexts[bossHpTextCursor];
        bossHpTextCursor = (bossHpTextCursor + 1) % MAX_BOSS_HP_TEXTS;
        text = CrowdControlBossHpText{};
        text.active = true;
        text.enemyPos = enemy.pos;
        text.kind = enemy.kind;
        std::snprintf(text.text, sizeof(text.text), "%d", std::max(0, hp));
    }

    void spawnDeathFx(const CrowdControlUnit &unit, bool malach)
    {
        if (!unit.active)
            return;
        sfxEvents.push(malach ? MiniGameSfxEvent::ANGEL_DIED : MiniGameSfxEvent::ENEMY_DIED);
        spawnParticleEvent(
            malach ? MiniGameParticleEventKind::ANGEL_DIED : MiniGameParticleEventKind::ENEMY_DIED,
            unit.pos,
            unit.lastMove,
            IsBoss(unit.kind) ? 1.0f : 0.62f
        );

        CrowdControlDeathFx &fx = deathFx[deathFxCursor];
        deathFxCursor = (deathFxCursor + 1) % MAX_DEATH_FX;
        fx = CrowdControlDeathFx{};
        fx.active = true;
        fx.malach = malach;
        fx.kind = unit.kind;
        fx.startPos = unit.pos;

        const bool boss = !malach && IsBoss(unit.kind);
        const float side = (unit.pos.x >= 0.0f ? 1.0f : -1.0f) * (nextRand01() < 0.35f ? -1.0f : 1.0f);
        fx.sideSign = side;
        fx.duration = boss ? (0.65f + nextRand01() * 0.20f) : (0.52f + nextRand01() * 0.18f);
        fx.distance = 1.0f + nextRand01() * 0.50f;
        fx.arcHeight = 0.32f + nextRand01() * 0.30f;
        fx.spin = (4.5f + nextRand01() * 4.5f) * side;

        if (boss)
        {
            fx.flyDir = glm::vec2(0.0f, 0.0f);
            return;
        }

        glm::vec2 opposite = -unit.lastMove;
        if (opposite.x * opposite.x + opposite.y * opposite.y < 1.0e-5f)
            opposite = malach ? glm::vec2(0.0f, -1.0f) : glm::vec2(0.0f, 1.0f);

        // Fly mostly sideways, with a smaller backward/forward kick opposite the last motion.
        glm::vec2 dir(side * (0.95f + nextRand01() * 0.55f), opposite.y * 0.45f + opposite.x * 0.12f);
        const float len2 = dir.x * dir.x + dir.y * dir.y;
        fx.flyDir = len2 > 1.0e-5f ? dir / std::sqrt(len2) : glm::vec2(side, 0.0f);

        fx.arcHeight *= DeathArcHeightMultiplier(unit.pos.x, fx.flyDir);
    }

    void updateDeathFx(float dt)
    {
        dt = std::clamp(dt, 0.0f, 0.05f);
        for (CrowdControlDeathFx &fx : deathFx)
        {
            if (!fx.active)
                continue;
            fx.age += dt;
            if (fx.age >= fx.duration)
                fx = CrowdControlDeathFx{};
        }
    }

    void updateFloatingTexts(float dt)
    {
        dt = std::clamp(dt, 0.0f, 0.05f);
        for (CrowdControlFloatingText &text : floatingTexts)
        {
            if (!text.active)
                continue;
            text.age += dt;
            if (text.age >= text.duration)
                text = CrowdControlFloatingText{};
        }
        for (CrowdControlBossHpText &text : bossHpTexts)
        {
            if (!text.active)
                continue;
            text.age += dt;
            if (text.age >= text.duration)
                text = CrowdControlBossHpText{};
        }
    }

    void clearMalachSlot(int index)
    {
        if (index < 0 || index >= MAX_MALACHIM)
            return;
        CrowdControlUnit &m = malachim[index];
        if (m.active)
            spawnDeathFx(m, true);
        if (m.active && m.pairedIndex >= 0 && m.pairedIndex < MAX_ENEMIES && enemies[m.pairedIndex].active)
        {
            enemies[m.pairedIndex].mode = CrowdControlUnitMode::MOVING;
            enemies[m.pairedIndex].pairedIndex = -1;
        }
        m = CrowdControlUnit{};
    }

    void clearEnemySlot(int index)
    {
        if (index < 0 || index >= MAX_ENEMIES)
            return;
        CrowdControlUnit &enemy = enemies[index];
        if (enemy.active)
            spawnDeathFx(enemy, false);
        if (enemy.active && enemy.pairedIndex >= 0 && enemy.pairedIndex < MAX_MALACHIM && malachim[enemy.pairedIndex].active)
        {
            malachim[enemy.pairedIndex].mode = CrowdControlUnitMode::MOVING;
            malachim[enemy.pairedIndex].pairedIndex = -1;
        }
        enemy = CrowdControlUnit{};
    }

    int nextMalachSpawnSlot()
    {
        const int index = malachSpawnCursor;
        malachSpawnCursor = (malachSpawnCursor + 1) % MAX_MALACHIM;
        clearMalachSlot(index);
        return index;
    }

    int nextEnemySpawnSlot()
    {
        const int index = enemySpawnCursor;
        enemySpawnCursor = (enemySpawnCursor + 1) % MAX_ENEMIES;
        clearEnemySlot(index);
        return index;
    }

    void addMissedCard(CrowdControlCardKind kind, float jsLen, int hp, int maxHp)
    {
        if (jsLen <= 0.0f)
            return;
        CrowdControlCard &card = missedCards[missedCardCursor];
        missedCardCursor = (missedCardCursor + 1) % MAX_MISSED_CARDS;
        card = CrowdControlCard{};
        card.active = true;
        card.pickable = false;
        card.kind = kind;
        card.pos.x = kind == CrowdControlCardKind::RATE ? ScreenLeftCorridorCenter() : ScreenRightCorridorCenter();
        card.pos.y = worldZFromJs(jsLen);
        card.hp = hp;
        card.maxHp = maxHp;
    }

    static inline bool IsRewardLaneX(float x)
    {
        return std::abs(x) >= MIDDLE_HALF_WIDTH;
    }

    static inline bool IsLeftRewardLaneX(float x)
    {
        return x > MIDDLE_HALF_WIDTH;
    }

    static inline bool IsRightRewardLaneX(float x)
    {
        return x < -MIDDLE_HALF_WIDTH;
    }

    static inline CrowdControlUnitLane LaneForX(float x)
    {
        if (IsLeftRewardLaneX(x))
            return CrowdControlUnitLane::LEFT_REWARD;
        if (IsRightRewardLaneX(x))
            return CrowdControlUnitLane::RIGHT_REWARD;
        return CrowdControlUnitLane::COMBAT;
    }

    static inline float malachSpawnZForBase(float snappedBaseX, int shotIndex, int spawnCursorBase)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        (void)snappedBaseX;
        (void)shotIndex;
        (void)spawnCursorBase;
        return LANE_START_Z + tuning.spawnMargin;
    }

    glm::vec2 malachSpawnPositionForShot(float snappedBaseX, int shotIndex, int spawnCursorBase) const
    {
        const float spawnZ = malachSpawnZForBase(snappedBaseX, shotIndex, spawnCursorBase);
        const float x = ClampReleaseXToSnappedCorridor(snappedBaseX, snappedBaseX);
        return glm::vec2(x, spawnZ);
    }

    bool hasRoomToSpawnMalachVolley(float snappedBaseX, int shots) const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float zClearance = tuning.unitRadius;
        (void)shots;
        const float spawnZ = malachSpawnZForBase(snappedBaseX, 0, malachSpawnCursor);
        for (const CrowdControlUnit &m : malachim)
        {
            if (!m.active)
                continue;
            if (std::abs(m.pos.y - spawnZ) <= zClearance)
                return false;
        }
        return true;
    }

    bool hasRoomToSpawnEnemy(float z) const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (const CrowdControlUnit &enemy : enemies)
        {
            if (enemy.active && !IsBoss(enemy.kind) && std::abs(enemy.pos.y - z) <= tuning.unitRadius)
                return false;
        }
        return true;
    }

    bool spawnMalach(glm::vec2 pos)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const int slot = nextMalachSpawnSlot();
        CrowdControlUnit &m = malachim[slot];
        const float ttl = myTtl;
        myBrown *= -1;
        m.active = true;
        m.pos = pos;
        m.lastMove = glm::vec2(0.0f, 1.0f);
        m.hp = HpFromFightStrength(ttl);
        m.maxHp = m.hp;
        m.fightStrength = ttl;
        m.maxFightStrength = ttl;
        const bool earlyDamageBoosted =
            totalMalachimSpawned < std::max(0, tuning.earlyAngelDamageBoostSpawnCount);
        m.hitBuff = myHitBuff * (earlyDamageBoosted ? tuning.earlyAngelDamageBoostMultiplier : 1.0f);
        m.kind = CrowdControlEnemyKind::DOG;
        m.mode = CrowdControlUnitMode::MOVING;
        m.fightTime = 0.0f;
        m.pairedIndex = -1;
        m.sidestepDir = myBrown;
        m.spawnSeq = nextSpawnSeq++;
        m.lane = LaneForX(pos.x);
        m.blocked = false;
        m.canFight = false;
        m.skipFirstMove = true;
        m.graceTime = tuning.graceTime;
        // JS keeps spawned angels at UNIT_SPEED even after left-lane upgrades.
        // The upgrade increases spawn rate only; using mySpeed here makes the
        // spawn row clear too fast and looks like the crowd is multiplying.
        m.speed = tuning.unitSpeed;
        ++totalMalachimSpawned;
        recordObservedMalachSpawn();
        return true;
    }

    bool spawnEnemy(CrowdControlEnemyKind kind)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const int slot = nextEnemySpawnSlot();
        CrowdControlUnit &enemy = enemies[slot];
        const float ttl = EnemyTtlForKind(kind, tuning) * themHealthBuff;
        themBrown *= -1;
        enemy.active = true;
        enemy.kind = kind;
        enemy.fightStrength = ttl;
        enemy.maxFightStrength = enemy.fightStrength;
        enemy.hp = HpFromFightStrength(enemy.fightStrength);
        enemy.maxHp = enemy.hp;
        enemy.hitBuff = themHitBuff;
        themHitBuff *= tuning.enemySpawnDamageMultiplier;
        themHealthBuff *= tuning.enemySpawnHealthMultiplier;
        enemy.mode = CrowdControlUnitMode::MOVING;
        enemy.fightTime = 0.0f;
        enemy.pairedIndex = -1;
        enemy.sidestepDir = themBrown;
        enemy.spawnSeq = nextSpawnSeq++;
        enemy.meleeCooldown = 0.0f;
        enemy.blocked = false;
        enemy.canFight = true;
        enemy.skipFirstMove = false;
        enemy.graceTime = 0.0f;
        enemy.speed = tuning.unitSpeed;
        const float sway = std::sin(lfo) * (MIDDLE_HALF_WIDTH - tuning.unitRadius);
        enemy.pos = glm::vec2(sway, worldZFromJs(tuning.spawnMargin));
        enemy.lastMove = glm::vec2(0.0f, -1.0f);
        ++totalEnemiesSpawned;
        if (IsBoss(kind))
            sfxEvents.push(MiniGameSfxEvent::BOSS_SPAWNED);
        return true;
    }

    void advanceWaveIfNeeded()
    {
        while (waveRemaining <= 0)
        {
            waveIndex = (waveIndex + 1) % DEFAULT_STREAM_COUNT;
            waveRemaining = DEFAULT_STREAM[waveIndex].count;
        }
        waveComplete = false;
    }

    bool frontlineIsOnEnemySide() const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float threshold = LANE_LENGTH * std::clamp(tuning.enemySideFrontlineThreshold01, 0.0f, 1.0f);
        return frontline < threshold;
    }

    float effectiveEnemySpawnRate() const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float multiplier = frontlineIsOnEnemySide()
            ? std::max(0.001f, tuning.enemySideFrontlineSpawnMultiplier)
            : 1.0f;
        return theirSpawnRate * multiplier;
    }

    void spawnEnemyStream(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        advanceWaveIfNeeded();
        if (enemyDelay > 0.0f)
        {
            enemyDelay -= dt;
            return;
        }

        float minAngelJs = LANE_LENGTH;
        for (const CrowdControlUnit &m : malachim)
            if (m.active)
                minAngelJs = std::min(minAngelJs, jsZFromWorld(m.pos.y));
        if (minAngelJs < tuning.spawnMargin + tuning.noSpawnIfCloserThan)
            return;

        enemySpawnTimer += dt;
        const float interval = 1.0f / std::max(effectiveEnemySpawnRate(), 0.001f);
        if (!enemyStarted || enemySpawnTimer > interval)
        {
            enemyStarted = true;
            enemySpawnTimer = 0.0f;
            const float spawnZ = worldZFromJs(tuning.spawnMargin);
            if (!hasRoomToSpawnEnemy(spawnZ))
                return;
            spawnEnemy(DEFAULT_STREAM[waveIndex].kind);
            --waveRemaining;
            advanceWaveIfNeeded();
        }
    }

    void decayOurSpawnRate(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float baseRate = std::max(tuning.ourSpawnRate, 0.001f);
        if (mySpawnRate <= baseRate)
        {
            mySpawnRate = baseRate;
            return;
        }

        const float decay = std::clamp(tuning.angelSpawnRateBoostDecayPerSecond * dt, 0.0f, 1.0f);
        mySpawnRate -= (mySpawnRate - baseRate) * decay;
        mySpawnRate = std::max(mySpawnRate, baseRate);
    }

    float malachSpawnIntervalSeconds() const
    {
        return 1.0f / std::max(mySpawnRate, 0.001f);
    }

    float spawnCubeBlink01() const
    {
        const float interval = malachSpawnIntervalSeconds();
        const float blinkWindow = std::min(0.5f, interval);
        if (blinkWindow <= 1e-6f)
            return 1.0f;
        return std::clamp((ourSpawnTimer - (interval - blinkWindow)) / blinkWindow, 0.0f, 1.0f);
    }

    void updateSpawnCubeVisualPulse()
    {
        if (malachSpawnIntervalSeconds() < 0.12f)
            spawnCubeFastBlinkOn = !spawnCubeFastBlinkOn;
        else
            spawnCubeFastBlinkOn = false;
    }

    float spawnCubeBlink01ForRender() const
    {
        if (malachSpawnIntervalSeconds() < 0.12f)
            return spawnCubeFastBlinkOn ? 1.0f : 0.0f;
        return spawnCubeBlink01();
    }

    void updateMalachSpawn(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        float maxEnemyJs = 0.0f;
        for (const CrowdControlUnit &enemy : enemies)
            if (enemy.active)
                maxEnemyJs = std::max(maxEnemyJs, jsZFromWorld(enemy.pos.y));
        if (maxEnemyJs > LANE_LENGTH - tuning.spawnMargin - tuning.noSpawnIfCloserThan)
            return;

        ourSpawnTimer += dt;
        const float interval = malachSpawnIntervalSeconds();
        if (weStarted && ourSpawnTimer <= interval)
            return;

        weStarted = true;
        const float snappedBaseX = SnapLaneControlX(spawnX);
        ourSpawnTimer = 0.0f;
        if (!hasRoomToSpawnMalachVolley(snappedBaseX, 1))
            return;

        spawnMalach(malachSpawnPositionForShot(snappedBaseX, 0, malachSpawnCursor));
    }

    void syncCardsFromBelts()
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (CrowdControlCard &card : cards)
            card = CrowdControlCard{};

        int write = 0;
        auto addCard = [&](CrowdControlCardKind kind, float jsLen, int hp, int maxHp, bool pickable)
        {
            if (write >= MAX_CARDS || jsLen <= 0.0f)
                return;
            CrowdControlCard &card = cards[write++];
            card.active = true;
            card.pickable = pickable;
            card.kind = kind;
            card.pos.x = kind == CrowdControlCardKind::RATE ? ScreenLeftCorridorCenter() : ScreenRightCorridorCenter();
            card.pos.y = worldZFromJs(jsLen);
            card.hp = hp;
            card.maxHp = maxHp;
        };

        // Missed cards are closest to the player and must not disappear exactly
        // when transitioning off the pickable conveyor, so give them render priority.
        for (const CrowdControlCard &missed : missedCards)
        {
            if (!missed.active || write >= MAX_CARDS)
                continue;
            cards[write++] = missed;
        }

        for (float len = leftBeltLen; len > 0.0f && write < MAX_CARDS; len -= tuning.leftUpgradeStep)
            addCard(CrowdControlCardKind::RATE, len, len == leftBeltLen ? leftBeltVal : tuning.leftUpgradePrice, tuning.leftUpgradePrice, true);
        for (float len = rightBeltLen; len > 0.0f && write < MAX_CARDS; len -= tuning.rightUpgradeStep)
            addCard(CrowdControlCardKind::POWER, len, len == rightBeltLen ? rightBeltVal : tuning.rightUpgradePrice, tuning.rightUpgradePrice, true);
    }

    float upgradeBeltSpeedMultiplier() const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        if (tuning.upgradeBeltSpeedBoostEnemySpawnPeriod <= 0)
            return 1.0f;
        const int steps = totalEnemiesSpawned / tuning.upgradeBeltSpeedBoostEnemySpawnPeriod;
        return std::pow(std::max(0.001f, tuning.upgradeBeltSpeedBoostMultiplier), (float)steps);
    }

    float leftUpgradeEffectiveSpeed() const
    {
        return CrowdControl_GetTuning().leftUpgradeSpeed * upgradeBeltSpeedMultiplier();
    }

    float rightUpgradeEffectiveSpeed() const
    {
        return CrowdControl_GetTuning().rightUpgradeSpeed * upgradeBeltSpeedMultiplier();
    }

    void moveCards(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float leftSpeed = leftUpgradeEffectiveSpeed();
        const float rightSpeed = rightUpgradeEffectiveSpeed();
        leftBeltLen += leftSpeed * dt;
        rightBeltLen += rightSpeed * dt;
        for (CrowdControlCard &card : missedCards)
        {
            if (!card.active)
                continue;
            const float speed = card.kind == CrowdControlCardKind::RATE
                ? leftSpeed
                : rightSpeed;
            card.pos.y -= speed * dt;
            if (jsZFromWorld(card.pos.y) >= MissedRewardExitJsZ(card.kind))
                card = CrowdControlCard{};
        }

        const float pickupEnd = LANE_LENGTH - tuning.spawnMargin;
        while (leftBeltLen > pickupEnd)
        {
            addMissedCard(CrowdControlCardKind::RATE, leftBeltLen, leftBeltVal, tuning.leftUpgradePrice);
            leftBeltLen -= tuning.leftUpgradeStep;
            leftBeltVal = tuning.leftUpgradePrice;
        }
        while (rightBeltLen > pickupEnd)
        {
            if (rightBeltVal > 0)
                sfxEvents.push(MiniGameSfxEvent::POWER_UPGRADE_MISSED);
            addMissedCard(CrowdControlCardKind::POWER, rightBeltLen, rightBeltVal, tuning.rightUpgradePrice);
            rightBeltLen -= tuning.rightUpgradeStep;
            rightBeltVal = tuning.rightUpgradePrice;
        }
        syncCardsFromBelts();
    }

    void recycleCard(CrowdControlCard &card)
    {
        (void)card;
        syncCardsFromBelts();
    }

    void applyCardHit(CrowdControlUnit &m, CrowdControlCard &card)
    {
        if (!m.active || !card.active || !card.pickable)
            return;
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const glm::vec2 d = m.pos - card.pos;
        if (d.x * d.x + d.y * d.y > tuning.unitRadius * tuning.unitRadius)
            return;
        const bool consumed = card.hp <= 1;
        spawnFloatingUpgradeText(card.pos, consumed ? card.maxHp : card.hp - 1, consumed);
        spawnUpgradeFeedback(card.kind, consumed, card.pos, card.pos - m.pos);
        spawnDeathFx(m, true);
        m.active = false;
        card.hp = std::max(0, card.hp - 1);
        if (card.kind == CrowdControlCardKind::RATE)
        {
            leftBeltVal = std::max(0, leftBeltVal - 1);
            if (leftBeltVal < 1)
            {
                leftBeltLen -= tuning.leftUpgradeStep;
                leftBeltVal = tuning.leftUpgradePrice;
                mySpeed *= tuning.angelSpeedUpgradeMultiplier;
                mySpawnRate *= tuning.angelSpawnRateUpgradeMultiplier;
                ++rateUpgrade;
            }
        }
        else
        {
            rightBeltVal = std::max(0, rightBeltVal - 1);
            if (rightBeltVal < 1)
            {
                rightBeltLen -= tuning.rightUpgradeStep;
                rightBeltVal = tuning.rightUpgradePrice;
                myTtl *= tuning.angelTtlUpgradeMultiplier;
                myHitBuff *= tuning.angelHitBuffUpgradeMultiplier;
                ++malachHealthUpgrade;
            }
        }
        syncCardsFromBelts();
    }

    void updateUpgradeHits()
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (CrowdControlUnit &m : malachim)
        {
            if (!m.active)
                continue;
            const float jsZ = jsZFromWorld(m.pos.y);
            if (m.lane == CrowdControlUnitLane::LEFT_REWARD && IsLeftRewardLaneX(m.pos.x))
            {
                if (jsZ < leftBeltLen)
                {
                    const bool consumed = leftBeltVal <= 1;
                    spawnFloatingUpgradeText(
                        glm::vec2(ScreenLeftCorridorCenter(), worldZFromJs(leftBeltLen)),
                        consumed ? tuning.leftUpgradePrice : leftBeltVal - 1,
                        consumed
                    );
                    spawnUpgradeFeedback(
                        CrowdControlCardKind::RATE,
                        consumed,
                        glm::vec2(ScreenLeftCorridorCenter(), worldZFromJs(leftBeltLen)),
                        glm::vec2(0.0f, -1.0f)
                    );
                    spawnDeathFx(m, true);
                    m.active = false;
                    leftBeltVal -= 1;
                    if (leftBeltVal < 1)
                    {
                        leftBeltLen -= tuning.leftUpgradeStep;
                        leftBeltVal = tuning.leftUpgradePrice;
                        mySpeed *= tuning.angelSpeedUpgradeMultiplier;
                        mySpawnRate *= tuning.angelSpawnRateUpgradeMultiplier;
                        ++rateUpgrade;
                    }
                }
                continue;
            }
            if (m.lane == CrowdControlUnitLane::RIGHT_REWARD && IsRightRewardLaneX(m.pos.x))
            {
                if (jsZ < rightBeltLen)
                {
                    const bool consumed = rightBeltVal <= 1;
                    spawnFloatingUpgradeText(
                        glm::vec2(ScreenRightCorridorCenter(), worldZFromJs(rightBeltLen)),
                        consumed ? tuning.rightUpgradePrice : rightBeltVal - 1,
                        consumed
                    );
                    spawnUpgradeFeedback(
                        CrowdControlCardKind::POWER,
                        consumed,
                        glm::vec2(ScreenRightCorridorCenter(), worldZFromJs(rightBeltLen)),
                        glm::vec2(0.0f, -1.0f)
                    );
                    spawnDeathFx(m, true);
                    m.active = false;
                    rightBeltVal -= 1;
                    if (rightBeltVal < 1)
                    {
                        rightBeltLen -= tuning.rightUpgradeStep;
                        rightBeltVal = tuning.rightUpgradePrice;
                        myTtl *= tuning.angelTtlUpgradeMultiplier;
                        myHitBuff *= tuning.angelHitBuffUpgradeMultiplier;
                        ++malachHealthUpgrade;
                    }
                }
                continue;
            }
        }
        syncCardsFromBelts();
    }

    void killEnemy(CrowdControlUnit &enemy)
    {
        if (!enemy.active)
            return;
        const int reward = EnemyMaxHp(enemy.kind);
        if (enemy.kind == CrowdControlEnemyKind::DOG)
            ++dogsKilled;
        else
        {
            ++bossesKilled;
            bossHpRewardEarned += reward;
        }
        rewardCoins += reward;
        spawnDeathFx(enemy, false);
        enemy = CrowdControlUnit{};
    }

    void unlinkMalachFromEnemy(int malachIndex)
    {
        if (malachIndex < 0 || malachIndex >= MAX_MALACHIM)
            return;
        const int enemyIndex = malachim[malachIndex].pairedIndex;
        if (enemyIndex >= 0 && enemyIndex < MAX_ENEMIES && enemies[enemyIndex].active && enemies[enemyIndex].pairedIndex == malachIndex)
        {
            enemies[enemyIndex].mode = CrowdControlUnitMode::MOVING;
            enemies[enemyIndex].pairedIndex = -1;
        }
        spawnDeathFx(malachim[malachIndex], true);
        malachim[malachIndex] = CrowdControlUnit{};
    }

    void unlinkEnemyFromMalach(int enemyIndex)
    {
        if (enemyIndex < 0 || enemyIndex >= MAX_ENEMIES)
            return;
        const int malachIndex = enemies[enemyIndex].pairedIndex;
        if (malachIndex >= 0 && malachIndex < MAX_MALACHIM && malachim[malachIndex].active && malachim[malachIndex].pairedIndex == enemyIndex)
        {
            malachim[malachIndex].mode = CrowdControlUnitMode::MOVING;
            malachim[malachIndex].pairedIndex = -1;
        }
        killEnemy(enemies[enemyIndex]);
    }

    void beginFightPair(int malachIndex, int enemyIndex)
    {
        CrowdControlUnit &m = malachim[malachIndex];
        CrowdControlUnit &enemy = enemies[enemyIndex];
        if (!m.active || !enemy.active || !m.canFight)
            return;
        if (m.mode != CrowdControlUnitMode::MOVING || enemy.mode != CrowdControlUnitMode::MOVING)
            return;
        m.mode = CrowdControlUnitMode::FIGHTING;
        m.pairedIndex = enemyIndex;
        m.blocked = false;
        enemy.mode = CrowdControlUnitMode::FIGHTING;
        enemy.pairedIndex = malachIndex;
        enemy.blocked = false;
        sfxEvents.push(MiniGameSfxEvent::FIGHT_START);
        spawnParticleEvent(
            MiniGameParticleEventKind::FIGHT_CONTACT,
            (m.pos + enemy.pos) * 0.5f,
            enemy.pos - m.pos,
            IsBoss(enemy.kind) ? 0.95f : 0.55f
        );
    }

    void beginNearbyFights()
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (int i = 0; i < MAX_MALACHIM; ++i)
        {
            CrowdControlUnit &m = malachim[i];
            if (!m.active || !m.canFight)
                continue;
            for (int j = 0; j < MAX_ENEMIES; ++j)
            {
                CrowdControlUnit &enemy = enemies[j];
                if (!enemy.active)
                    continue;
                if (std::abs(m.pos.y - enemy.pos.y) >= 2.0f * tuning.unitRadius)
                    continue;
                if (std::abs(m.pos.x - enemy.pos.x) >= 2.0f * tuning.unitRadius)
                    continue;
                if (IsBoss(enemy.kind))
                    continue;
                if (enemy.pairedIndex < 0 && m.pairedIndex < 0 &&
                    enemy.mode == CrowdControlUnitMode::MOVING &&
                    m.mode == CrowdControlUnitMode::MOVING)
                {
                    beginFightPair(i, j);
                }
                else
                {
                    enemy.blocked = true;
                    m.blocked = true;
                }
            }
        }
    }

    void updateBossContacts(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        const float contact = 2.0f * tuning.unitRadius;
        const float smashRadius2 = tuning.bossSmashRadius * tuning.bossSmashRadius;
        for (int j = 0; j < MAX_ENEMIES; ++j)
        {
            CrowdControlUnit &boss = enemies[j];
            if (!boss.active || !IsBoss(boss.kind))
                continue;

            bool anyContact = false;
            const int maxTargets = boss.kind == CrowdControlEnemyKind::THRONE
                ? tuning.throneSmashMaxTargets
                : tuning.seraphSmashMaxTargets;
            const float smashDamage = (boss.kind == CrowdControlEnemyKind::THRONE
                ? tuning.throneSmashDamage
                : tuning.seraphSmashDamage) * boss.hitBuff;

            for (int i = 0; i < MAX_MALACHIM; ++i)
            {
                CrowdControlUnit &m = malachim[i];
                if (!m.active || !m.canFight || m.lane != CrowdControlUnitLane::COMBAT)
                    continue;
                if (std::abs(m.pos.y - boss.pos.y) >= contact)
                    continue;
                if (std::abs(m.pos.x - boss.pos.x) >= contact)
                    continue;

                anyContact = true;
                m.blocked = true;
                m.bossBlocked = true;
                m.mode = CrowdControlUnitMode::FIGHTING;
                m.pairedIndex = j;
                m.fightTime += dt;

                const int hpBefore = boss.hp;
                boss.fightStrength -= dt * m.hitBuff;
                boss.hp = HpFromFightStrength(boss.fightStrength);
                if (boss.hp < hpBefore)
                    spawnBossHpText(boss, boss.hp);
            }

            if (!anyContact || boss.meleeCooldown > 0.0f)
                continue;

            std::array<int, MAX_BOSS_SMASH_TARGETS> targets{};
            const int targetLimit = std::clamp(maxTargets, 0, MAX_BOSS_SMASH_TARGETS);
            int targetCount = 0;
            for (int pick = 0; pick < targetLimit; ++pick)
            {
                int best = -1;
                float bestDist2 = smashRadius2;
                for (int i = 0; i < MAX_MALACHIM; ++i)
                {
                    const CrowdControlUnit &m = malachim[i];
                    if (!m.active || m.lane != CrowdControlUnitLane::COMBAT)
                        continue;
                    bool alreadyPicked = false;
                    for (int k = 0; k < targetCount; ++k)
                        alreadyPicked = alreadyPicked || targets[k] == i;
                    if (alreadyPicked)
                        continue;
                    const glm::vec2 delta = m.pos - boss.pos;
                    const float dist2 = delta.x * delta.x + delta.y * delta.y;
                    if (dist2 <= bestDist2)
                    {
                        bestDist2 = dist2;
                        best = i;
                    }
                }
                if (best < 0)
                    break;
                targets[targetCount++] = best;
            }

            if (targetCount <= 0)
                continue;

            boss.meleeCooldown = boss.kind == CrowdControlEnemyKind::THRONE
                ? THRONE_MELEE_COOLDOWN_S
                : SERAPH_MELEE_COOLDOWN_S;
            sfxEvents.push(MiniGameSfxEvent::FIGHT_START);
            spawnParticleEvent(
                MiniGameParticleEventKind::BOSS_SMASH,
                boss.pos,
                glm::vec2(0.0f, -1.0f),
                boss.kind == CrowdControlEnemyKind::THRONE ? 1.0f : 0.82f
            );
            for (int k = 0; k < targetCount; ++k)
            {
                CrowdControlUnit &m = malachim[targets[k]];
                if (!m.active)
                    continue;
                m.fightStrength -= smashDamage;
                m.hp = HpFromFightStrength(m.fightStrength);
                m.fightTime += 0.05f;
                spawnParticleEvent(
                    MiniGameParticleEventKind::FIGHT_CONTACT,
                    (m.pos + boss.pos) * 0.5f,
                    m.pos - boss.pos,
                    boss.kind == CrowdControlEnemyKind::THRONE ? 1.0f : 0.72f
                );
            }
        }
    }

    void updateOwnTeamBlocking()
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (int i = 0; i < MAX_MALACHIM; ++i)
        {
            CrowdControlUnit &m = malachim[i];
            if (!m.active)
                continue;
            const bool wasBlockedByEnemy = m.blocked;
            bool blockedByOwn = false;
            for (int j = i - 1; j >= 0; --j)
            {
                const CrowdControlUnit &other = malachim[j];
                if (other.active &&
                    std::abs(m.pos.y - other.pos.y) < 2.0f * tuning.unitRadius &&
                    std::abs(m.pos.x - other.pos.x) < 2.0f * tuning.unitRadius)
                {
                    blockedByOwn = true;
                    break;
                }
            }
            m.blocked = blockedByOwn || wasBlockedByEnemy;
        }

        for (int i = 0; i < MAX_ENEMIES; ++i)
        {
            CrowdControlUnit &enemy = enemies[i];
            if (!enemy.active)
                continue;
            const bool wasBlockedByEnemy = enemy.blocked;
            bool blockedByOwn = false;
            for (int j = i - 1; j >= 0; --j)
            {
                const CrowdControlUnit &other = enemies[j];
                if (other.active &&
                    !IsBoss(enemy.kind) &&
                    !IsBoss(other.kind) &&
                    std::abs(enemy.pos.y - other.pos.y) < 2.0f * tuning.unitRadius &&
                    std::abs(enemy.pos.x - other.pos.x) < 2.0f * tuning.unitRadius)
                {
                    blockedByOwn = true;
                    break;
                }
            }
            enemy.blocked = blockedByOwn || wasBlockedByEnemy;
        }
    }

    void moveUnit(CrowdControlUnit &unit, float dt, float direction)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        if (unit.mode == CrowdControlUnitMode::FIGHTING)
            return;
        if (unit.bossBlocked)
            return;
        const glm::vec2 before = unit.pos;
        if (unit.blocked)
        {
            const float combatCenterLimit = CombatCenterLimit(tuning);
            if (nextRand01() < tuning.chanceToTurnAroundPerSecond * dt)
                unit.sidestepDir *= -1;
            const float newX = unit.pos.x + tuning.brownSpeed * (float)unit.sidestepDir * dt;
            if (newX > -combatCenterLimit && newX < combatCenterLimit)
                unit.pos.x = newX;
            else
                unit.sidestepDir *= -1;
        }
        else
        {
            unit.pos.y += direction * unit.speed * dt;
        }
        const glm::vec2 delta = unit.pos - before;
        if (delta.x * delta.x + delta.y * delta.y > 1.0e-8f)
            unit.lastMove = delta;
    }

    void stopMalachAtBossBarrier(CrowdControlUnit &m, float beforeY)
    {
        for (const CrowdControlUnit &boss : enemies)
        {
            if (!boss.active || !IsBoss(boss.kind))
                continue;
            if (boss.pos.y < beforeY)
                continue;
            if (m.pos.y < boss.pos.y)
                continue;
            // Bosses lock the whole combat corridor: malachim can attack the
            // boss line, but cannot route around it without killing the boss.
            m.pos.y = beforeY;
            m.blocked = true;
            m.bossBlocked = true;
            return;
        }
    }

    bool frontmostCombatMalachY(float *outY) const
    {
        float frontY = -1.0e9f;
        bool found = false;
        for (const CrowdControlUnit &m : malachim)
        {
            if (!m.active || m.lane != CrowdControlUnitLane::COMBAT)
                continue;
            frontY = std::max(frontY, m.pos.y);
            found = true;
        }
        if (found && outY)
            *outY = frontY;
        return found;
    }

    float bossMovementDirection(const CrowdControlUnit &boss) const
    {
        if (!boss.active || !IsBoss(boss.kind))
            return -1.0f;
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        float frontY = 0.0f;
        if (!frontmostCombatMalachY(&frontY))
            return 0.0f;
        const float deadZone = tuning.unitRadius;
        if (frontY > boss.pos.y + deadZone)
            return 1.0f;
        if (frontY < boss.pos.y - deadZone)
            return -1.0f;
        return 0.0f;
    }

    void updateMovement(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (CrowdControlUnit &enemy : enemies)
        {
            if (!enemy.active)
                continue;
            if (enemy.meleeCooldown > 0.0f)
                enemy.meleeCooldown = std::max(0.0f, enemy.meleeCooldown - dt);
            const float direction = bossMovementDirection(enemy);
            moveUnit(enemy, dt, direction);
            if (enemy.pos.y <= LANE_START_Z + tuning.spawnMargin)
            {
                enemy = CrowdControlUnit{};
                --fortressHp;
                if (fortressHp <= 0)
                    endReason = CrowdControlEndReason::ENEMY_REACHED_SPAWN;
            }
        }

        for (CrowdControlUnit &m : malachim)
        {
            if (!m.active)
                continue;
            if (m.graceTime > 0.0f)
            {
                m.graceTime -= dt;
                if (m.graceTime <= 0.0f)
                    m.canFight = true;
            }
            if (m.skipFirstMove)
            {
                m.skipFirstMove = false;
                continue;
            }
            const float beforeY = m.pos.y;
            moveUnit(m, dt, 1.0f);
            if (m.active && m.lane == CrowdControlUnitLane::COMBAT)
                stopMalachAtBossBarrier(m, beforeY);
            if (m.pos.y > LANE_END_Z + tuning.spawnMargin)
            {
                if (m.lane == CrowdControlUnitLane::COMBAT)
                {
                    phase = CrowdControlPhase::WON;
                    endReason = CrowdControlEndReason::MALACH_REACHED_ENEMY_BASE;
                    return;
                }
                // Reward lanes are upgrade conveyors, not win lanes. If a side
                // runner reaches the far end without hitting a card, discard it.
                spawnDeathFx(m, true);
                m = CrowdControlUnit{};
                continue;
            }
        }
    }

    void updateFrontline()
    {
        float maxEnemyJs = 0.0f;
        float minAngelJs = LANE_LENGTH;
        for (const CrowdControlUnit &m : malachim)
            if (m.active)
                minAngelJs = std::min(minAngelJs, jsZFromWorld(m.pos.y));
        for (const CrowdControlUnit &enemy : enemies)
            if (enemy.active)
                maxEnemyJs = std::max(maxEnemyJs, jsZFromWorld(enemy.pos.y));
        frontline = (maxEnemyJs + minAngelJs) * 0.5f;
    }

    void updateFights(float dt)
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        for (CrowdControlUnit &enemy : enemies)
        {
            if (enemy.active)
            {
                enemy.blocked = false;
                enemy.bossBlocked = false;
            }
        }
        for (CrowdControlUnit &m : malachim)
        {
            if (m.active)
            {
                if (m.mode == CrowdControlUnitMode::FIGHTING &&
                    m.pairedIndex >= 0 &&
                    m.pairedIndex < MAX_ENEMIES &&
                    enemies[m.pairedIndex].active &&
                    IsBoss(enemies[m.pairedIndex].kind))
                {
                    m.mode = CrowdControlUnitMode::MOVING;
                    m.pairedIndex = -1;
                }
                m.blocked = false;
                m.bossBlocked = false;
            }
        }

        updateBossContacts(dt);
        beginNearbyFights();
        updateOwnTeamBlocking();

        for (CrowdControlUnit &enemy : enemies)
        {
            if (!enemy.active)
                continue;
            const int hpBefore = enemy.hp;
            if (enemy.mode == CrowdControlUnitMode::FIGHTING)
            {
                float incomingHitBuff = 1.0f;
                const int malachIndex = enemy.pairedIndex;
                if (malachIndex >= 0 && malachIndex < MAX_MALACHIM && malachim[malachIndex].active)
                    incomingHitBuff = malachim[malachIndex].hitBuff;
                enemy.fightStrength -= dt * incomingHitBuff;
                enemy.fightTime += dt;
            }
            if (enemy.blocked)
                enemy.fightStrength -= dt * tuning.blockedDamagePerSecond;
            enemy.hp = HpFromFightStrength(enemy.fightStrength);
            if (IsBoss(enemy.kind) && enemy.hp < hpBefore)
                spawnBossHpText(enemy, enemy.hp);
        }

        for (CrowdControlUnit &m : malachim)
        {
            if (!m.active)
                continue;
            if (m.mode == CrowdControlUnitMode::FIGHTING)
            {
                float incomingHitBuff = 1.0f;
                const int enemyIndex = m.pairedIndex;
                if (enemyIndex >= 0 && enemyIndex < MAX_ENEMIES && enemies[enemyIndex].active)
                {
                    if (IsBoss(enemies[enemyIndex].kind))
                        continue;
                    incomingHitBuff = enemies[enemyIndex].hitBuff;
                }
                m.fightStrength -= dt * incomingHitBuff;
                m.fightTime += dt;
            }
            if (m.blocked)
                m.fightStrength -= dt * tuning.blockedDamagePerSecond;
            m.hp = HpFromFightStrength(m.fightStrength);
        }

        for (int i = 0; i < MAX_ENEMIES; ++i)
        {
            if (enemies[i].active && enemies[i].fightStrength <= 0.0f)
                unlinkEnemyFromMalach(i);
        }
        for (int i = 0; i < MAX_MALACHIM; ++i)
        {
            if (malachim[i].active && malachim[i].fightStrength <= 0.0f)
                unlinkMalachFromEnemy(i);
        }
    }

    int validFightingPairCount() const
    {
        int count = 0;
        for (int i = 0; i < MAX_MALACHIM; ++i)
        {
            const CrowdControlUnit &m = malachim[i];
            if (!m.active || m.mode != CrowdControlUnitMode::FIGHTING)
                continue;
            const int enemyIndex = m.pairedIndex;
            if (enemyIndex >= 0 && enemyIndex < MAX_ENEMIES &&
                enemies[enemyIndex].active &&
                enemies[enemyIndex].mode == CrowdControlUnitMode::FIGHTING &&
                enemies[enemyIndex].pairedIndex == i)
            {
                ++count;
            }
        }
        return count;
    }

    int contactCandidateCount() const
    {
        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        int count = 0;
        for (const CrowdControlUnit &m : malachim)
        {
            if (!m.active)
                continue;
            for (const CrowdControlUnit &enemy : enemies)
            {
                if (enemy.active &&
                    std::abs(m.pos.y - enemy.pos.y) < 2.0f * tuning.unitRadius &&
                    std::abs(m.pos.x - enemy.pos.x) < 2.0f * tuning.unitRadius)
                {
                    ++count;
                }
            }
        }
        return count;
    }

    float totalEnemyFightStrength() const
    {
        float total = 0.0f;
        for (const CrowdControlUnit &enemy : enemies)
            if (enemy.active)
                total += enemy.fightStrength;
        return total;
    }

    void dumpStaleState(const char *reason, int contactCandidates) const
    {
        std::printf("CROWD_CONTROL_STALE_DUMP_BEGIN reason=%s\n", reason ? reason : "unknown");
        std::printf(
            "summary elapsed=%.3f us=%d enemy=%d destroyed=%d dogs=%d bosses=%d reward=%d "
            "pairs=%d contact=%d waveIndex=%d waveRemaining=%d rate=%d ttl=%.3f hit=%.3f spawnPerMin=%.3f "
            "frontline=%.3f targetX=%.3f spawnX=%.3f leftLen=%.3f leftVal=%d rightLen=%.3f rightVal=%d\n",
            elapsed,
            activeMalachCount(),
            activeEnemyCount(),
            destroyedEnemyCount(),
            dogsKilled,
            bossesKilled,
            rewardCoins,
            validFightingPairCount(),
            contactCandidates,
            waveIndex,
            waveRemaining,
            rateUpgrade,
            newMalachTtlSeconds(),
            newMalachHitBuff(),
            spawnedMalachimPerMinute(),
            frontline,
            targetX,
            spawnX,
            leftBeltLen,
            leftBeltVal,
            rightBeltLen,
            rightBeltVal
        );
        for (int i = 0; i < MAX_MALACHIM; ++i)
        {
            const CrowdControlUnit &m = malachim[i];
            if (!m.active)
                continue;
            std::printf(
                "M slot=%d mode=%d x=%.3f z=%.3f ttl=%.3f hit=%.3f fight=%.3f pair=%d seq=%u side=%d blocked=%d canFight=%d grace=%.3f\n",
                i, (int)m.mode, m.pos.x, m.pos.y, m.fightStrength, m.hitBuff,
                m.fightTime, m.pairedIndex, m.spawnSeq, m.sidestepDir, m.blocked ? 1 : 0,
                m.canFight ? 1 : 0, m.graceTime
            );
        }
        for (int i = 0; i < MAX_ENEMIES; ++i)
        {
            const CrowdControlUnit &enemy = enemies[i];
            if (!enemy.active)
                continue;
            std::printf(
                "E slot=%d kind=%d mode=%d x=%.3f z=%.3f ttl=%.3f hit=%.3f fight=%.3f pair=%d seq=%u side=%d blocked=%d\n",
                i, (int)enemy.kind, (int)enemy.mode, enemy.pos.x, enemy.pos.y,
                enemy.fightStrength, enemy.hitBuff, enemy.fightTime, enemy.pairedIndex,
                enemy.spawnSeq, enemy.sidestepDir, enemy.blocked ? 1 : 0
            );
        }
        std::printf("CROWD_CONTROL_STALE_DUMP_END\n");
    }

    void detectAndDumpStaleState(float dt)
    {
        const int destroyed = destroyedEnemyCount();
        const float enemyStrength = totalEnemyFightStrength();
        if (destroyed != staleLastDestroyedCount || std::abs(enemyStrength - staleLastEnemyStrength) > 0.001f)
        {
            staleLastDestroyedCount = destroyed;
            staleLastEnemyStrength = enemyStrength;
            staleNoKillTimer = 0.0f;
            staleDumpedThisEpisode = false;
            return;
        }
        const int contacts = contactCandidateCount();
        if (contacts <= 0 || activeMalachCount() <= 0 || activeEnemyCount() <= 0)
        {
            staleNoKillTimer = 0.0f;
            staleDumpedThisEpisode = false;
            return;
        }
        staleNoKillTimer += dt;
        if (staleNoKillTimer >= 2.0f && !staleDumpedThisEpisode)
        {
            dumpStaleState("js_port_contact_without_ttl_progress_for_2s", contacts);
            staleDumpedThisEpisode = true;
        }
    }

    void updateCrowdControl(float dt, float inputX)
    {
        updateDeathFx(dt);
        updateFloatingTexts(dt);
        if (phase != CrowdControlPhase::RUNNING)
            return;

        const CrowdControlTuning tuning = CrowdControl_GetTuning();
        dt = std::clamp(dt, 0.0f, 0.05f);
        updateSpawnCubeVisualPulse();

        // ----- INPUT TARGET -----
        // JS reads keyboard state into ctx.mySpawnX. In-game controls provide the
        // desired lane X directly, then we pass it through the same snap function.
        const float rawInputX = std::clamp(inputX, -LANE_HALF_WIDTH, LANE_HALF_WIDTH);
        targetX = SnapLaneControlX(rawInputX);
        if (waitingForFirstInput)
            return;

        // ----- TIME / LFO -----
        // JS: delta, ctx.lfo, ctx.lfo2. Rendering owns its own animation clocks,
        // so gameplay only advances the spawn sway LFO here.
        elapsed += dt;
        lfo += dt;
        decayOurSpawnRate(dt);

        // ----- UPGRADE BELTS -----
        // JS: leftBeltLen/rightBeltLen move and wrap before input movement/spawns.
        moveCards(dt);

        // ----- CONTROLLED SPAWN POSITION -----
        // JS: left/right keys move ctx.mySpawnX at ctrlSpeed then getSnappedSpawnX()
        // is used at fire time. Here touch/mouse already picked targetX; we only
        // ease the center combat corridor while snapping reward lanes immediately.
        const bool targetIsRewardLane = std::abs(targetX) >= MIDDLE_HALF_WIDTH;
        if (targetIsRewardLane)
        {
            // Do not ease through the combat-corridor clamp: reward lanes live in the outer 1/6.
            spawnX = targetX;
        }
        else
        {
            const float follow = std::clamp(dt * tuning.inputFollowSpeed, 0.0f, 1.0f);
            spawnX += (targetX - spawnX) * follow;
            spawnX = SnapLaneControlX(spawnX);
        }

        // ----- FRONTLINE SCAN FOR SPAWN GATES -----
        // JS computes maxEnemy/minAngel before spawning. The spawn helpers rescan
        // the same values internally; this keeps the debug frontline readable too.
        updateFrontline();

        // ----- SPAWN ENEMIES -----
        // JS: enemyDelay, "we winning" gate, enemy timer, ring-buffer spawn.
        spawnEnemyStream(dt);

        // ----- SPAWN ANGELS -----
        // JS: "we losing" gate, our timer, spawn-row capacity, ring-buffer spawn.
        updateMalachSpawn(dt);

        // ----- MOVEMENT -----
        // JS: enemies move toward us, angels move toward enemy; fighting units stop,
        // blocked units sidestep in the middle corridor.
        updateMovement(dt);
        if (phase != CrowdControlPhase::RUNNING)
            return;

        // ----- DESIRED FRONTLINE -----
        // JS updates ctx.frontline after movement. We keep the same debug value.
        updateFrontline();

        // ----- LEFT / RIGHT REWARD LANES -----
        // JS consumes angels in the outer 1/6 lanes after movement.
        updateUpgradeHits();

        // ----- COLLISIONS, OWN-TEAM BLOCKING, TTL, REMOVAL -----
        // JS does these as four consecutive sections; updateFights preserves that
        // order internally to keep the port easy to compare.
        updateFights(dt);

        // ----- STALE DEBUG / END CONDITIONS -----
        detectAndDumpStaleState(dt);

        if (fortressHp <= 0)
        {
            phase = CrowdControlPhase::LOST;
            if (endReason == CrowdControlEndReason::NONE)
                endReason = CrowdControlEndReason::ENEMY_REACHED_SPAWN;
        }
    }

    void tick(float dt, float inputX)
    {
        updateCrowdControl(dt, inputX);
    }

    std::array<CrowdControlCardLabel, CARD_LABEL_SLOTS> visibleCardLabels() const
    {
        std::array<CrowdControlCardLabel, CARD_LABEL_SLOTS> labels{};
        for (int i = 0; i < MAX_CARDS; ++i)
        {
            if (!cards[i].active || !cards[i].pickable)
                continue;
            labels[i].active = true;
            labels[i].cardIndex = i;
            const int value = cards[i].hp;
            std::snprintf(labels[i].text, sizeof(labels[i].text), "+%d", value);
        }
        return labels;
    }

    int labelSlotForCardIndex(int cardIndex) const
    {
        if (cardIndex < 0 || cardIndex >= CARD_LABEL_SLOTS || !cards[cardIndex].active || !cards[cardIndex].pickable)
            return -1;
        const CrowdControlCard &card = cards[cardIndex];
        if (card.kind == CrowdControlCardKind::RATE)
            return card.hp >= card.maxHp ? 0 : 1;
        return card.hp >= card.maxHp ? 2 : 3;
    }

    bool isDone() const
    {
        return phase == CrowdControlPhase::WON || phase == CrowdControlPhase::LOST;
    }
};

static inline void initCrowdControl(CrowdControlState &state)
{
    state.initCrowdControl();
}

static inline void updateCrowdControl(CrowdControlState &state, float dt, float inputX)
{
    state.updateCrowdControl(dt, inputX);
}
