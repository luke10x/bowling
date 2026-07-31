#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <string>

#include "../minigames/coin_rush/coin_rush.h"
#include "../minigames/count_masters/count_masters.h"
#include "../minigames/crowd_control/crowd_control.h"

TEST_CASE("Coin rush minigame lays out sparse moving coin rows")
{
    CoinLane lane = {};
    MiniGameCoinRush::InitCoinGrid(&lane);

    REQUIRE(lane.activeCount == MiniGameCoinRush::ComputeCoinCount());
    REQUIRE(lane.activeCount > 20);
    REQUIRE(lane.activeCount < 60);
    CHECK(lane.currentPattern == CoinPattern::SideToSide);
    CHECK(lane.visualKind == CollectableVisualKind::Coin);
    CHECK(lane.deployedGemCount == 0);

    const int columns = MiniGameCoinRush::ComputeColumnCount();
    const int rows = MiniGameCoinRush::ComputeRowCount();
    REQUIRE(columns == MiniGameCoinRush::SLOT_COUNT);
    REQUIRE(rows >= 10);
    REQUIRE(rows <= 22);

    const Coin &first = lane.coins[0];
    const Coin &second = lane.coins[1];
    const Coin &last = lane.coins[lane.activeCount - 1];

    CHECK(first.state == CoinState::Active);
    CHECK(first.visualKind == CollectableVisualKind::Coin);
    CHECK(second.position.x > first.position.x);
    CHECK(second.position.z == doctest::Approx(first.position.z));
    CHECK(first.phaseOffset > 0.0f);
    CHECK(first.position.z >= CoinLane::LANE_START_Z);
    CHECK(last.position.z <= CoinLane::LANE_END_Z);

    int counted = 0;
    for (int row = 0; row < rows; ++row)
    {
        int rowCount = 0;
        for (int slot = 0; slot < columns; ++slot)
            rowCount += MiniGameCoinRush::SlotHasCoin(row, slot) ? 1 : 0;
        CHECK(rowCount >= MiniGameCoinRush::MIN_COINS_PER_ROW);
        CHECK(rowCount <= MiniGameCoinRush::MAX_COINS_PER_ROW);
        counted += rowCount;
    }
    CHECK(counted == lane.activeCount);
}

TEST_CASE("Campaign victories pick story bonus minigames by defeated opponent tier")
{
    CHECK(MiniGame_BonusForCampaignVictory(1, false, false) == MiniGameKind::NONE);
    CHECK(MiniGame_BonusForCampaignVictory(1, true, false) == MiniGameKind::NONE);
    CHECK(MiniGame_BonusForCampaignVictory(2, true, false) == MiniGameKind::COIN_RUSH);
    CHECK(MiniGame_BonusForCampaignVictory(5, true, false) == MiniGameKind::COIN_RUSH);
    CHECK(MiniGame_BonusForCampaignVictory(6, true, true) == MiniGameKind::COUNT_MASTERS);
    CHECK(MiniGame_BonusForCampaignVictory(8, true, true) == MiniGameKind::COUNT_MASTERS);
    CHECK(MiniGame_BonusForCampaignVictory(9, true, true, true) == MiniGameKind::CROWD_CONTROL);
    CHECK(MiniGame_BonusForCampaignVictory(12, true, true, true) == MiniGameKind::CROWD_CONTROL);
    CHECK(MiniGame_BonusForCampaignVictory(13, true, true, true) == MiniGameKind::NONE);
}

TEST_CASE("Crowd Control starts paused and fires only after first input")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CHECK(state.waitingForFirstInput);
    CHECK(state.activeMalachCount() == 0);

    state.updateCrowdControl(1.0f, 0.25f);
    CHECK(state.elapsed == doctest::Approx(0.0f));
    CHECK(state.activeMalachCount() == 0);
    CHECK(state.targetX == doctest::Approx(0.25f));

    state.waitingForFirstInput = false;
    for (int i = 0; i < 10; ++i)
        state.updateCrowdControl(0.05f, 0.25f);
    CHECK(state.elapsed > 0.0f);
    CHECK(state.activeMalachCount() > 0);
}

TEST_CASE("Crowd Control snaps control lanes like the JS prototype")
{
    const float screenLeftSideCenter = CrowdControlState::ScreenLeftCorridorCenter();
    const float screenRightSideCenter = CrowdControlState::ScreenRightCorridorCenter();
    const float leftCombatEdge = -CrowdControlState::LANE_HALF_WIDTH + CrowdControlState::LANE_WIDTH * 3.0f / 12.0f;
    const float rightCombatEdge = -CrowdControlState::LANE_HALF_WIDTH + CrowdControlState::LANE_WIDTH * 9.0f / 12.0f;
    const float leftOuterThreshold = -CrowdControlState::LANE_HALF_WIDTH + CrowdControlState::LANE_WIDTH * 2.0f / 12.0f;
    const float rightOuterThreshold = -CrowdControlState::LANE_HALF_WIDTH + CrowdControlState::LANE_WIDTH * 10.0f / 12.0f;

    CHECK(CrowdControlState::SnapLaneControlX(-0.49f) == doctest::Approx(screenRightSideCenter));
    CHECK(CrowdControlState::SnapLaneControlX(0.49f) == doctest::Approx(screenLeftSideCenter));
    CHECK(CrowdControlState::SnapLaneControlX(leftOuterThreshold - 0.01f) == doctest::Approx(screenRightSideCenter));
    CHECK(CrowdControlState::SnapLaneControlX(rightOuterThreshold + 0.01f) == doctest::Approx(screenLeftSideCenter));
    CHECK(CrowdControlState::SnapLaneControlX(leftCombatEdge + 0.01f) == doctest::Approx(leftCombatEdge + 0.01f));
    CHECK(CrowdControlState::SnapLaneControlX(rightCombatEdge - 0.01f) == doctest::Approx(rightCombatEdge - 0.01f));
    CHECK(CrowdControlState::SnapLaneControlX(leftCombatEdge - 0.01f) == doctest::Approx(leftCombatEdge));
    CHECK(CrowdControlState::SnapLaneControlX(rightCombatEdge + 0.01f) == doctest::Approx(rightCombatEdge));
    CHECK(CrowdControlState::SnapLaneControlX(0.10f) == doctest::Approx(0.10f));
}

TEST_CASE("Crowd Control can steer spawn point into outer reward lanes")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;

    const float leftSideCenter = CrowdControlState::ScreenLeftCorridorCenter();
    const float rightSideCenter = CrowdControlState::ScreenRightCorridorCenter();

    state.spawnX = 0.0f;
    state.updateCrowdControl(0.016f, CrowdControlState::LANE_HALF_WIDTH);
    CHECK(state.targetX == doctest::Approx(leftSideCenter));
    CHECK(state.spawnX == doctest::Approx(leftSideCenter));

    state.updateCrowdControl(0.016f, -CrowdControlState::LANE_HALF_WIDTH);
    CHECK(state.targetX == doctest::Approx(rightSideCenter));
    CHECK(state.spawnX == doctest::Approx(rightSideCenter));
}

TEST_CASE("Crowd Control starts at halved spawn rates")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CHECK(CrowdControl_GetTuning().ourSpawnRate == doctest::Approx(2.0f));
    CHECK(state.mySpawnRate == doctest::Approx(CrowdControl_GetTuning().ourSpawnRate));
    CHECK(state.theirSpawnRate == doctest::Approx(CrowdControl_GetTuning().enemySpawnRate));
}

TEST_CASE("Crowd Control enemy damage and health grow every spawn")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));
    CHECK(state.enemies[0].hitBuff == doctest::Approx(1.0f));
    CHECK(state.enemies[0].fightStrength == doctest::Approx(CrowdControl_GetTuning().enemyStartingTtl));
    CHECK(state.themHitBuff == doctest::Approx(CrowdControl_GetTuning().enemyStartingHitBuff *
                                               CrowdControl_GetTuning().enemySpawnDamageMultiplier));
    CHECK(state.themHealthBuff == doctest::Approx(CrowdControl_GetTuning().enemyStartingHealthBuff *
                                                  CrowdControl_GetTuning().enemySpawnHealthMultiplier));

    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));
    CHECK(state.enemies[1].hitBuff == doctest::Approx(CrowdControl_GetTuning().enemyStartingHitBuff *
                                                      CrowdControl_GetTuning().enemySpawnDamageMultiplier));
    CHECK(state.enemies[1].fightStrength == doctest::Approx(CrowdControl_GetTuning().enemyStartingTtl *
                                                            CrowdControl_GetTuning().enemySpawnHealthMultiplier));
    CHECK(state.themHitBuff == doctest::Approx(CrowdControl_GetTuning().enemyStartingHitBuff *
                                               CrowdControl_GetTuning().enemySpawnDamageMultiplier *
                                               CrowdControl_GetTuning().enemySpawnDamageMultiplier));
    CHECK(state.themHealthBuff == doctest::Approx(CrowdControl_GetTuning().enemyStartingHealthBuff *
                                                  CrowdControl_GetTuning().enemySpawnHealthMultiplier *
                                                  CrowdControl_GetTuning().enemySpawnHealthMultiplier));
}

TEST_CASE("Crowd Control first spawned malachim get early damage boost")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.myHitBuff = 2.0f;
    state.totalMalachimSpawned = tuning.earlyAngelDamageBoostSpawnCount - 1;
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    CHECK(state.malachim[0].hitBuff == doctest::Approx(2.0f * tuning.earlyAngelDamageBoostMultiplier));
    CHECK(state.totalMalachimSpawned == tuning.earlyAngelDamageBoostSpawnCount);

    REQUIRE(state.spawnMalach(glm::vec2(0.1f, 0.0f)));
    CHECK(state.malachim[1].hitBuff == doctest::Approx(2.0f));
    CHECK(state.totalMalachimSpawned == tuning.earlyAngelDamageBoostSpawnCount + 1);
}

TEST_CASE("Crowd Control boosted spawn rate visibly decays to minimum")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.mySpawnRate = tuning.ourSpawnRate * 4.0f;
    state.spawnRateDecayGraceLeft = 0.0f;
    const float boostedRate = state.mySpawnRate;
    state.decayOurSpawnRate(1.0f);
    CHECK(state.mySpawnRate < boostedRate);
    CHECK(state.mySpawnRate > CrowdControlState::SpawnRatePerSecondFromPerMinute(tuning.angelSpawnRateMinPerMinute));

    state.decayOurSpawnRate(1000.0f);
    CHECK(state.mySpawnRate == doctest::Approx(
        CrowdControlState::SpawnRatePerSecondFromPerMinute(tuning.angelSpawnRateMinPerMinute)
    ));
}

TEST_CASE("Crowd Control rate-card boost noticeably fades during gameplay")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.mySpawnRate = tuning.ourSpawnRate * tuning.angelSpawnRateUpgradeMultiplier;
    state.spawnRateDecayGraceLeft = 0.0f;
    const float boostedRate = state.mySpawnRate;

    for (int i = 0; i < 100; ++i)
        state.updateCrowdControl(0.05f, 0.0f);

    CHECK(state.mySpawnRate < boostedRate);
    CHECK(state.mySpawnRate >= CrowdControlState::SpawnRatePerSecondFromPerMinute(tuning.angelSpawnRateMinPerMinute));
}

TEST_CASE("Crowd Control spawn capability only grows from upgrades then decays")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.mySpawnRate = tuning.ourSpawnRate * 3.0f;
    state.spawnRateDecayGraceLeft = 0.0f;
    const float boostedRate = state.mySpawnRate;
    state.updateCrowdControl(0.25f, 0.0f);
    CHECK(state.mySpawnRate < boostedRate);
    CHECK(state.mySpawnRate >= CrowdControlState::SpawnRatePerSecondFromPerMinute(tuning.angelSpawnRateMinPerMinute));

    state.leftBeltVal = 1;
    CrowdControlUnit left = {};
    left.active = true;
    left.lane = CrowdControlUnitLane::LEFT_REWARD;
    left.pos = glm::vec2(
        CrowdControlState::ScreenLeftCorridorCenter(),
        CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f)
    );
    state.malachim[0] = left;
    const float beforeUpgrade = state.mySpawnRate;
    state.updateUpgradeHits();
    CHECK(state.mySpawnRate > beforeUpgrade);

    const float afterUpgrade = state.mySpawnRate;
    state.updateCrowdControl(0.25f, 0.0f);
    CHECK(state.mySpawnRate == doctest::Approx(afterUpgrade));
    const int ticksPastGrace = (int)((tuning.angelSpawnRateDecayGraceSeconds + 0.25f) / 0.05f) + 2;
    for (int i = 0; i < ticksPastGrace; ++i)
        state.updateCrowdControl(0.05f, 0.0f);
    CHECK(state.mySpawnRate < afterUpgrade);
}

TEST_CASE("Crowd Control dashboard spawn rate uses configured spawn capability")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.mySpawnRate = 2.0f;

    CHECK(state.spawnedMalachimPerMinute() == doctest::Approx(120.0f));

    state.mySpawnRate = 7.0f;
    CHECK(state.spawnedMalachimPerMinute() == doctest::Approx(420.0f));
}

TEST_CASE("Crowd Control left and right reward lanes are screen-relative")
{
    CHECK(CrowdControlState::ScreenLeftCorridorCenter() > 0.0f);
    CHECK(CrowdControlState::ScreenRightCorridorCenter() < 0.0f);
    CHECK(CrowdControlState::LaneForX(CrowdControlState::ScreenLeftCorridorCenter()) == CrowdControlUnitLane::LEFT_REWARD);
    CHECK(CrowdControlState::LaneForX(CrowdControlState::ScreenRightCorridorCenter()) == CrowdControlUnitLane::RIGHT_REWARD);
}

TEST_CASE("Crowd Control reward lane releases and blocked combat bodies sidestep like JS")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.spawnX = CrowdControlState::ScreenLeftCorridorCenter();

    state.updateMalachSpawn(1.0f);
    int spawned = 0;
    for (const CrowdControlUnit &m : state.malachim)
    {
        if (!m.active)
            continue;
        ++spawned;
        CHECK(m.pos.x == doctest::Approx(CrowdControlState::ScreenLeftCorridorCenter()));
    }
    CHECK(spawned > 0);

    CrowdControlUnit blocked = {};
    blocked.active = true;
    blocked.blocked = true;
    blocked.sidestepDir = 1;
    blocked.speed = CrowdControl_GetTuning().unitSpeed;
    blocked.pos = glm::vec2(0.0f, -10.0f);
    state.moveUnit(blocked, 1.0f, 1.0f);
    CHECK(blocked.pos.x == doctest::Approx(CrowdControl_GetTuning().brownSpeed));
    CHECK(blocked.pos.y == doctest::Approx(-10.0f));
}

TEST_CASE("Crowd Control pairs only the front battle line")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    CrowdControlUnit &farMalach = state.malachim[0];
    farMalach.active = true;
    farMalach.canFight = true;
    farMalach.lane = CrowdControlUnitLane::COMBAT;
    farMalach.mode = CrowdControlUnitMode::MOVING;
    farMalach.pos = glm::vec2(0.0f, CrowdControlState::LANE_START_Z + 2.0f);
    farMalach.fightStrength = tuning.ourStartingTtl;

    CrowdControlUnit &farEnemy = state.enemies[0];
    farEnemy.active = true;
    farEnemy.kind = CrowdControlEnemyKind::DOG;
    farEnemy.mode = CrowdControlUnitMode::MOVING;
    farEnemy.pos = glm::vec2(0.0f, CrowdControlState::LANE_START_Z + 3.0f);
    farEnemy.fightStrength = tuning.enemyStartingTtl;

    state.beginFrontlineDogFights();
    CHECK(farMalach.mode == CrowdControlUnitMode::MOVING);
    CHECK(farEnemy.mode == CrowdControlUnitMode::MOVING);

    farMalach.pos = glm::vec2(-0.20f, CrowdControlState::LANE_START_Z + 2.50f);
    farEnemy.pos = glm::vec2(0.20f, CrowdControlState::LANE_START_Z + 2.55f);
    state.beginFrontlineDogFights();

    CHECK(farMalach.mode == CrowdControlUnitMode::FIGHTING);
    CHECK(farEnemy.mode == CrowdControlUnitMode::FIGHTING);
    CHECK(farMalach.pairedIndex == 0);
    CHECK(farEnemy.pairedIndex == 0);
}

TEST_CASE("Crowd Control batches front line dog pairing across frames")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    for (CrowdControlUnit &m : state.malachim)
        m = CrowdControlUnit{};
    for (CrowdControlUnit &e : state.enemies)
        e = CrowdControlUnit{};

    for (int i = 0; i < 2; ++i)
    {
        CrowdControlUnit &m = state.malachim[i];
        m.active = true;
        m.canFight = true;
        m.lane = CrowdControlUnitLane::COMBAT;
        m.mode = CrowdControlUnitMode::MOVING;
        m.pos = glm::vec2(0.0f, 0.01f * float(i));
        m.fightStrength = tuning.ourStartingTtl;

        CrowdControlUnit &enemy = state.enemies[i];
        enemy.active = true;
        enemy.kind = CrowdControlEnemyKind::DOG;
        enemy.mode = CrowdControlUnitMode::MOVING;
        enemy.pos = glm::vec2(0.0f, 0.08f + 0.01f * float(i));
        enemy.fightStrength = tuning.enemyStartingTtl;
    }

    state.beginFrontlineDogFights(/*batched=*/true);
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.malachim[1].mode == CrowdControlUnitMode::MOVING);
    CHECK(state.enemies[1].mode == CrowdControlUnitMode::MOVING);

    state.beginFrontlineDogFights(/*batched=*/true);
    CHECK(state.malachim[1].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[1].mode == CrowdControlUnitMode::FIGHTING);
}

TEST_CASE("Crowd Control uses JS pressure gates near each base")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;

    state.enemies[0].active = true;
    state.enemies[0].kind = CrowdControlEnemyKind::DOG;
    state.enemies[0].pos = glm::vec2(0.0f, CrowdControlState::LANE_START_Z + 2.75f);
    state.enemies[0].fightStrength = 10.0f;

    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() > 0);

    CrowdControlState blocked = {};
    blocked.initCrowdControl();
    blocked.waitingForFirstInput = false;
    blocked.weStarted = true;
    blocked.ourSpawnTimer = 10.0f;

    blocked.enemies[0].active = true;
    blocked.enemies[0].kind = CrowdControlEnemyKind::DOG;
    blocked.enemies[0].pos = glm::vec2(0.0f, CrowdControlState::LANE_START_Z + 2.25f);
    blocked.enemies[0].fightStrength = 10.0f;

    blocked.updateMalachSpawn(0.0f);
    CHECK(blocked.activeMalachCount() == 0);
    CHECK(blocked.ourSpawnTimer == doctest::Approx(10.0f));

    CrowdControlState enemyBlocked = {};
    enemyBlocked.initCrowdControl();
    enemyBlocked.waitingForFirstInput = false;
    enemyBlocked.enemySpawnTimer = 10.0f;
    enemyBlocked.enemyDelay = 0.0f;

    enemyBlocked.malachim[0].active = true;
    enemyBlocked.malachim[0].pos = glm::vec2(0.0f, CrowdControlState::LANE_END_Z - 2.25f);

    enemyBlocked.spawnEnemyStream(0.0f);
    CHECK(enemyBlocked.activeEnemyCount() == 0);
}

TEST_CASE("Crowd Control enemy spawn rate boosts when frontline is on enemy side")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.theirSpawnRate = 2.0f;
    state.frontline = CrowdControlState::LANE_LENGTH * 0.75f;
    CHECK_FALSE(state.frontlineIsOnEnemySide());
    CHECK(state.effectiveEnemySpawnRate() == doctest::Approx(2.0f));

    state.frontline = CrowdControlState::LANE_LENGTH * (tuning.enemySideFrontlineThreshold01 - 0.05f);
    CHECK(state.frontlineIsOnEnemySide());
    CHECK(state.effectiveEnemySpawnRate() == doctest::Approx(2.0f * tuning.enemySideFrontlineSpawnMultiplier));
}

TEST_CASE("Crowd Control wins when a malach reaches the enemy end")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;
    state.enemySpawnTimer = 10.0f;
    state.enemyDelay = 0.0f;

    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    state.malachim[10].active = true;
    state.malachim[10].pos = glm::vec2(0.0f, CrowdControlState::LANE_END_Z - 1.25f);
    state.malachim[10].speed = tuning.unitSpeed;
    state.malachim[10].fightStrength = 10.0f;
    state.malachim[10].maxFightStrength = 10.0f;

    state.spawnEnemyStream(0.0f);
    CHECK(state.activeEnemyCount() == 0);

    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() > 1);

    state.malachim[10].pos.y = CrowdControlState::LANE_END_Z + tuning.spawnMargin - 0.01f;
    state.updateMovement(0.05f);
    CHECK(state.phase == CrowdControlPhase::WON);
    CHECK(state.endReason == CrowdControlEndReason::MALACH_REACHED_ENEMY_BASE);
}

TEST_CASE("Crowd Control does not consume spawn timer when spawn point is temporarily blocked")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;

    const float spawnZ = CrowdControlState::LANE_START_Z + CrowdControl_GetTuning().spawnMargin;
    state.malachim[0].active = true;
    state.malachim[0].pos = glm::vec2(0.0f, spawnZ);

    state.updateMalachSpawn(0.0f);
    CHECK(state.ourSpawnTimer == doctest::Approx(10.0f));

    state.malachim[0].active = false;
    state.ourSpawnTimer = 10.0f;
    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() > 0);
    CHECK(state.ourSpawnTimer == doctest::Approx(0.0f));
}

TEST_CASE("Crowd Control spawn room is capped by occupied spawn row like JS")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;

    const float spawnZ = CrowdControlState::LANE_START_Z + CrowdControl_GetTuning().spawnMargin;
    state.malachim[10].active = true;
    state.malachim[10].pos = glm::vec2(CrowdControlState::ScreenLeftCorridorCenter(), spawnZ);
    state.spawnX = 0.0f;

    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() == 1);
    CHECK(state.ourSpawnTimer == doctest::Approx(10.0f));
}

TEST_CASE("Crowd Control freshly spawned malach renders at spawn point for first frame")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    const float spawnZ = CrowdControlState::LANE_START_Z + tuning.spawnMargin;

    REQUIRE(state.spawnMalach(glm::vec2(0.0f, spawnZ)));
    REQUIRE(state.malachim[0].active);

    state.updateMovement(0.05f);
    CHECK(state.malachim[0].pos.y == doctest::Approx(spawnZ));

    state.updateMovement(0.05f);
    CHECK(state.malachim[0].pos.y > spawnZ);
}

TEST_CASE("Crowd Control spawn rate is capped by occupied spawn band")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.mySpawnRate = 10000.0f;
    state.ourSpawnTimer = 10.0f;

    state.updateMalachSpawn(0.0f);
    const int spawnedFirstVolley = state.activeMalachCount();
    REQUIRE(spawnedFirstVolley == 1);

    state.ourSpawnTimer = 10.0f;
    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() == spawnedFirstVolley);
    CHECK(state.ourSpawnTimer == doctest::Approx(10.0f));

    for (CrowdControlUnit &m : state.malachim)
        if (m.active)
            m.pos.y += CrowdControl_GetTuning().unitRadius * 4.0f;

    state.ourSpawnTimer = 10.0f;
    state.updateMalachSpawn(0.0f);
    CHECK(state.activeMalachCount() > spawnedFirstVolley);
}

TEST_CASE("Crowd Control spawn cube pulse ramps over spawn interval")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    state.mySpawnRate = 1.0f;
    state.ourSpawnTimer = 0.25f;
    CHECK(state.spawnCubeBlink01() == doctest::Approx(0.0f));
    state.ourSpawnTimer = 0.75f;
    CHECK(state.spawnCubeBlink01() == doctest::Approx(0.5f));
    state.ourSpawnTimer = 1.0f;
    CHECK(state.spawnCubeBlink01() == doctest::Approx(1.0f));

    state.mySpawnRate = 4.0f;
    state.ourSpawnTimer = 0.125f;
    CHECK(state.spawnCubeBlink01() == doctest::Approx(0.5f));

    state.mySpawnRate = 20.0f;
    state.ourSpawnTimer = 0.0f;
    CHECK(state.spawnCubeBlink01() == doctest::Approx(0.0f));
    CHECK(state.spawnCubeBlink01ForRender() == doctest::Approx(0.0f));
    state.updateSpawnCubeVisualPulse();
    CHECK(state.spawnCubeBlink01ForRender() == doctest::Approx(1.0f));
    state.updateSpawnCubeVisualPulse();
    CHECK(state.spawnCubeBlink01ForRender() == doctest::Approx(0.0f));
}

TEST_CASE("Crowd Control screen controls map through centered portrait box on wide screens")
{
    const float logicalW = 1280.0f;
    const float logicalH = 720.0f;
    const float portraitW = logicalH * 9.0f / 16.0f;
    const float portraitLeft = (logicalW - portraitW) * 0.5f;
    const float portraitRight = portraitLeft + portraitW;

    CHECK(CrowdControlState::PortraitMappedX01(portraitLeft - 20.0f, logicalW, logicalH) == doctest::Approx(0.0f));
    CHECK(CrowdControlState::PortraitMappedX01(portraitRight + 20.0f, logicalW, logicalH) == doctest::Approx(1.0f));
    CHECK(CrowdControlState::PortraitMappedX01(portraitLeft + portraitW * 0.5f, logicalW, logicalH) == doctest::Approx(0.5f));

    CHECK(CrowdControlState::ScreenXToLaneX(
        portraitLeft - 20.0f,
        logicalW,
        logicalH,
        CrowdControlState::LANE_HALF_WIDTH
    ) == doctest::Approx(CrowdControlState::LANE_HALF_WIDTH));
    CHECK(CrowdControlState::ScreenXToLaneX(
        portraitRight + 20.0f,
        logicalW,
        logicalH,
        CrowdControlState::LANE_HALF_WIDTH
    ) == doctest::Approx(-CrowdControlState::LANE_HALF_WIDTH));
}

TEST_CASE("Crowd Control keeps angels and enemies in fixed ring buffers")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    for (int i = 0; i < CrowdControlState::MAX_MALACHIM + 3; ++i)
        REQUIRE(state.spawnMalach(glm::vec2(0.01f * (float)i, CrowdControlState::LANE_START_Z)));
    CHECK(state.activeMalachCount() == CrowdControlState::MAX_MALACHIM);
    CHECK(state.malachSpawnCursor == 3);

    for (int i = 0; i < CrowdControlState::MAX_ENEMIES + 5; ++i)
        REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));
    CHECK(state.activeEnemyCount() == CrowdControlState::MAX_ENEMIES);
    CHECK(state.enemySpawnCursor == 5);
}

TEST_CASE("Crowd Control belts move continuously and render all active labels")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    const float leftBefore = state.leftBeltLen;
    const float rightBefore = state.rightBeltLen;
    state.moveCards(1.0f);

    CHECK(state.leftBeltLen > leftBefore);
    CHECK(state.rightBeltLen > rightBefore);

    int activeCards = 0;
    int activeLabels = 0;
    int pickableCards = 0;
    const auto labels = state.visibleCardLabels();
    for (int i = 0; i < CrowdControlState::MAX_CARDS; ++i)
    {
        activeCards += state.cards[i].active ? 1 : 0;
        pickableCards += state.cards[i].active && state.cards[i].pickable ? 1 : 0;
        activeLabels += labels[i].active ? 1 : 0;
        if (state.cards[i].active)
        {
            const int slot = state.labelSlotForCardIndex(i);
            if (state.cards[i].pickable)
            {
                CHECK(slot >= 0);
                CHECK(slot < 4);
            }
            else
            {
                CHECK(slot == -1);
            }
        }
    }
    CHECK(activeCards > 0);
    CHECK(activeLabels == pickableCards);
}

TEST_CASE("Crowd Control upgrade belts speed up every enemy spawn block")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.totalEnemiesSpawned = tuning.upgradeBeltSpeedBoostEnemySpawnPeriod - 1;
    CHECK(state.upgradeBeltSpeedMultiplier() == doctest::Approx(1.0f));
    CHECK(state.leftUpgradeEffectiveSpeed() == doctest::Approx(tuning.leftUpgradeSpeed));

    state.totalEnemiesSpawned = tuning.upgradeBeltSpeedBoostEnemySpawnPeriod;
    CHECK(state.upgradeBeltSpeedMultiplier() == doctest::Approx(tuning.upgradeBeltSpeedBoostMultiplier));
    CHECK(state.rightUpgradeEffectiveSpeed() == doctest::Approx(tuning.rightUpgradeSpeed * tuning.upgradeBeltSpeedBoostMultiplier));

    state.totalEnemiesSpawned = tuning.upgradeBeltSpeedBoostEnemySpawnPeriod * 2;
    CHECK(state.upgradeBeltSpeedMultiplier() == doctest::Approx(tuning.upgradeBeltSpeedBoostMultiplier *
                                                                tuning.upgradeBeltSpeedBoostMultiplier));

    const float leftBefore = state.leftBeltLen;
    const float rightBefore = state.rightBeltLen;
    state.moveCards(1.0f);
    CHECK(state.leftBeltLen - leftBefore == doctest::Approx(tuning.leftUpgradeSpeed * state.upgradeBeltSpeedMultiplier()));
    CHECK(state.rightBeltLen - rightBefore == doctest::Approx(tuning.rightUpgradeSpeed * state.upgradeBeltSpeedMultiplier()));
}

TEST_CASE("Crowd Control missed rewards stay visible without labels or pickup")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    const float pickupEnd = CrowdControlState::LANE_LENGTH - tuning.spawnMargin;
    state.leftBeltLen = pickupEnd + 0.01f;
    state.leftBeltVal = tuning.leftUpgradePrice;

    state.moveCards(0.0f);

    bool foundMissed = false;
    int missedIndex = -1;
    for (int i = 0; i < CrowdControlState::MAX_CARDS; ++i)
    {
        const CrowdControlCard &card = state.cards[i];
        if (!card.active || card.pickable || card.kind != CrowdControlCardKind::RATE)
            continue;
        foundMissed = true;
        missedIndex = i;
        CHECK(CrowdControlState::jsZFromWorld(card.pos.y) > pickupEnd);
        CHECK(state.labelSlotForCardIndex(i) == -1);
    }
    REQUIRE(foundMissed);

    CrowdControlUnit runner = {};
    runner.active = true;
    runner.lane = CrowdControlUnitLane::LEFT_REWARD;
    runner.pos = state.cards[missedIndex].pos;
    state.malachim[0] = runner;

    state.updateUpgradeHits();

    CHECK(state.malachim[0].active);
    CHECK(state.rateUpgrade == 0);
}

TEST_CASE("Crowd Control missed rewards fade for 20cm after lane end before despawn")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    const float laneEnd = CrowdControlState::MissedRewardLaneEndJsZ(CrowdControlCardKind::RATE);
    const float exit = CrowdControlState::MissedRewardExitJsZ(CrowdControlCardKind::RATE);
    CHECK(exit == doctest::Approx(laneEnd + tuning.missedRewardExitTailDistance));
    CHECK(CrowdControlState::MissedRewardShouldRender(
        CrowdControlCardKind::RATE,
        laneEnd - 0.01f
    ));
    CHECK(CrowdControlState::MissedRewardAlpha(CrowdControlCardKind::RATE, laneEnd - 0.01f) == doctest::Approx(1.0f));

    state.addMissedCard(
        CrowdControlCardKind::RATE,
        laneEnd + tuning.missedRewardExitTailDistance * 0.5f,
        1,
        1
    );
    state.moveCards(0.0f);

    bool foundMissedNearExit = false;
    for (const CrowdControlCard &card : state.cards)
    {
        if (!card.active || card.pickable)
            continue;
        const float jsZ = CrowdControlState::jsZFromWorld(card.pos.y);
        foundMissedNearExit |= jsZ > CrowdControlState::MissedRewardLaneEndJsZ(card.kind);
    }
    CHECK(foundMissedNearExit);
    CHECK(CrowdControlState::MissedRewardShouldRender(
        CrowdControlCardKind::RATE,
        laneEnd + tuning.missedRewardExitTailDistance * 0.5f
    ));
    CHECK(CrowdControlState::MissedRewardAlpha(
        CrowdControlCardKind::RATE,
        laneEnd + tuning.missedRewardExitTailDistance * 0.5f
    ) == doctest::Approx(0.5f));

    state.missedCards[0].pos.y = CrowdControlState::worldZFromJs(
        exit
    );
    state.moveCards(0.0f);
    CHECK_FALSE(state.missedCards[0].active);
}

TEST_CASE("Crowd Control missed rewards keep render priority during belt transition")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    const float pickupEnd = CrowdControlState::LANE_LENGTH - tuning.spawnMargin;

    state.leftBeltLen = pickupEnd + 0.01f;
    state.rightBeltLen = pickupEnd + 0.01f;
    for (int i = 0; i < CrowdControlState::MAX_MISSED_CARDS; ++i)
    {
        state.addMissedCard(
            (i & 1) == 0 ? CrowdControlCardKind::RATE : CrowdControlCardKind::POWER,
            pickupEnd + 0.02f + 0.01f * float(i),
            1,
            1
        );
    }

    state.moveCards(0.0f);

    int visibleMissed = 0;
    for (const CrowdControlCard &card : state.cards)
        if (card.active && !card.pickable)
            ++visibleMissed;

    CHECK(visibleMissed == CrowdControlState::MAX_MISSED_CARDS);
}

TEST_CASE("Crowd Control left and right reward conveyors consume malachim")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit left = {};
    left.active = true;
    left.lane = CrowdControlUnitLane::LEFT_REWARD;
    left.pos = glm::vec2(CrowdControlState::ScreenLeftCorridorCenter(), CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f));
    state.malachim[0] = left;
    state.updateUpgradeHits();
    CHECK_FALSE(state.malachim[0].active);
    CHECK(state.rateUpgrade == 1);
    CHECK(state.mySpawnRate == doctest::Approx(CrowdControl_GetTuning().ourSpawnRate *
                                               CrowdControl_GetTuning().angelSpawnRateUpgradeMultiplier));
    CHECK(state.floatingTexts[0].active);
    CHECK(state.floatingTexts[0].consumed);
    CHECK(std::string(state.floatingTexts[0].text) == "1");
    CHECK(state.floatingTexts[0].cardPos.x == doctest::Approx(CrowdControlState::ScreenLeftCorridorCenter()));
    REQUIRE(state.particleEvents.count >= 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::UPGRADE_CONSUMED);

    state.particleEvents.clear();
    state.sfxEvents.clear();
    state.rightBeltVal = 1;
    CrowdControlUnit right = {};
    right.active = true;
    right.lane = CrowdControlUnitLane::RIGHT_REWARD;
    right.pos = glm::vec2(CrowdControlState::ScreenRightCorridorCenter(), CrowdControlState::worldZFromJs(state.rightBeltLen - 0.01f));
    state.malachim[1] = right;
    state.updateUpgradeHits();
    CHECK_FALSE(state.malachim[1].active);
    CHECK(state.malachHealthUpgrade == 1);
    CHECK(state.myTtl == doctest::Approx(CrowdControl_GetTuning().ourStartingTtl *
                                          CrowdControl_GetTuning().angelTtlUpgradeMultiplier));
    CHECK(state.myHitBuff == doctest::Approx(CrowdControl_GetTuning().ourStartingHitBuff *
                                             CrowdControl_GetTuning().angelHitBuffUpgradeMultiplier));
    CHECK(state.floatingTexts[1].active);
    CHECK(state.floatingTexts[1].consumed);
    CHECK(std::string(state.floatingTexts[1].text) == "99");
    CHECK(state.floatingTexts[1].cardPos.x == doctest::Approx(CrowdControlState::ScreenRightCorridorCenter()));
    REQUIRE(state.particleEvents.count >= 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::POWER_UPGRADE_CONSUMED);
    REQUIRE(state.sfxEvents.count >= 1);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::POWER_UPGRADE_CONSUMED);
}

TEST_CASE("Crowd Control missed power upgrade plays miss cue")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    state.sfxEvents.clear();
    state.rightBeltLen = CrowdControlState::LANE_LENGTH - tuning.spawnMargin + 0.01f;
    state.rightBeltVal = tuning.rightUpgradePrice;
    state.moveCards(0.0f);

    REQUIRE(state.sfxEvents.count >= 1);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::POWER_UPGRADE_MISSED);
}

TEST_CASE("Crowd Control upgrade hits spawn non-consuming floating text and expire")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.leftBeltVal = 2;

    CrowdControlUnit left = {};
    left.active = true;
    left.lane = CrowdControlUnitLane::LEFT_REWARD;
    left.pos = glm::vec2(
        CrowdControlState::ScreenLeftCorridorCenter(),
        CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f)
    );
    state.malachim[0] = left;

    state.updateUpgradeHits();

    CHECK_FALSE(state.malachim[0].active);
    CHECK(state.rateUpgrade == 0);
    REQUIRE(state.floatingTexts[0].active);
    CHECK_FALSE(state.floatingTexts[0].consumed);
    CHECK(std::string(state.floatingTexts[0].text) == "1");
    CHECK(state.floatingTexts[0].cardPos.y == doctest::Approx(CrowdControlState::worldZFromJs(state.leftBeltLen)));
    REQUIRE(state.particleEvents.count >= 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::UPGRADE_HIT);

    for (int i = 0; i < 40; ++i)
        state.updateFloatingTexts(0.05f);
    CHECK_FALSE(state.floatingTexts[0].active);
}

TEST_CASE("Crowd Control power upgrade floating text shows remaining value")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.rightBeltVal = 99;

    CrowdControlUnit right = {};
    right.active = true;
    right.lane = CrowdControlUnitLane::RIGHT_REWARD;
    right.pos = glm::vec2(
        CrowdControlState::ScreenRightCorridorCenter(),
        CrowdControlState::worldZFromJs(state.rightBeltLen - 0.01f)
    );
    state.malachim[0] = right;

    state.updateUpgradeHits();

    CHECK_FALSE(state.malachim[0].active);
    CHECK(state.malachHealthUpgrade == 0);
    REQUIRE(state.floatingTexts[0].active);
    CHECK_FALSE(state.floatingTexts[0].consumed);
    CHECK(std::string(state.floatingTexts[0].text) == "98");
    REQUIRE(state.particleEvents.count >= 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::UPGRADE_HIT);
}

TEST_CASE("Crowd Control middle corridor never consumes reward conveyors")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.leftBeltVal = 1;
    state.rightBeltVal = 1;
    const float rewardZ = CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f);

    CrowdControlUnit center = {};
    center.active = true;
    center.pos = glm::vec2(0.0f, rewardZ);
    state.malachim[0] = center;

    CrowdControlUnit leftFightEdge = {};
    leftFightEdge.active = true;
    leftFightEdge.pos = glm::vec2(
        -CrowdControlState::MIDDLE_HALF_WIDTH + 0.001f,
        rewardZ
    );
    state.malachim[1] = leftFightEdge;

    CrowdControlUnit rightFightEdge = {};
    rightFightEdge.active = true;
    rightFightEdge.pos = glm::vec2(
        CrowdControlState::MIDDLE_HALF_WIDTH - 0.001f,
        rewardZ
    );
    state.malachim[2] = rightFightEdge;

    state.updateUpgradeHits();

    CHECK(state.malachim[0].active);
    CHECK(state.malachim[1].active);
    CHECK(state.malachim[2].active);
    CHECK(state.rateUpgrade == 0);
    CHECK(state.malachHealthUpgrade == 0);
    CHECK(state.mySpawnRate == doctest::Approx(CrowdControl_GetTuning().ourSpawnRate));
    CHECK(state.myTtl == doctest::Approx(CrowdControl_GetTuning().ourStartingTtl));
}

TEST_CASE("Crowd Control combat-spawned angels cannot collect reward even if displaced sideways")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.leftBeltVal = 1;
    state.rightBeltVal = 1;

    CrowdControlUnit combatOnLeft = {};
    combatOnLeft.active = true;
    combatOnLeft.lane = CrowdControlUnitLane::COMBAT;
    combatOnLeft.pos = glm::vec2(
        CrowdControlState::ScreenLeftCorridorCenter(),
        CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f)
    );
    state.malachim[0] = combatOnLeft;

    CrowdControlUnit combatOnRight = {};
    combatOnRight.active = true;
    combatOnRight.lane = CrowdControlUnitLane::COMBAT;
    combatOnRight.pos = glm::vec2(
        CrowdControlState::ScreenRightCorridorCenter(),
        CrowdControlState::worldZFromJs(state.rightBeltLen - 0.01f)
    );
    state.malachim[1] = combatOnRight;

    state.updateUpgradeHits();

    CHECK(state.malachim[0].active);
    CHECK(state.malachim[1].active);
    CHECK(state.rateUpgrade == 0);
    CHECK(state.malachHealthUpgrade == 0);
}

TEST_CASE("Crowd Control left reward does not speed up newly spawned angels")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const float originalUnitSpeed = CrowdControl_GetTuning().unitSpeed;

    CrowdControlUnit left = {};
    left.active = true;
    left.lane = CrowdControlUnitLane::LEFT_REWARD;
    left.pos = glm::vec2(
        CrowdControlState::ScreenLeftCorridorCenter(),
        CrowdControlState::worldZFromJs(state.leftBeltLen - 0.01f)
    );
    state.malachim[0] = left;
    state.updateUpgradeHits();

    CHECK(state.rateUpgrade == 1);
    CHECK(state.mySpawnRate == doctest::Approx(CrowdControl_GetTuning().ourSpawnRate *
                                               CrowdControl_GetTuning().angelSpawnRateUpgradeMultiplier));
    CHECK(state.mySpeed > originalUnitSpeed);

    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;
    state.spawnX = 0.0f;
    state.updateMalachSpawn(0.0f);

    bool foundSpawn = false;
    for (const CrowdControlUnit &m : state.malachim)
    {
        if (!m.active)
            continue;
        foundSpawn = true;
        CHECK(m.speed == doctest::Approx(originalUnitSpeed));
    }
    CHECK(foundSpawn);
}

TEST_CASE("Crowd Control reward lane spawn does not inherit combat cursor offset")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    state.weStarted = true;
    state.ourSpawnTimer = 10.0f;
    state.malachSpawnCursor = 900;
    state.spawnX = CrowdControlState::ScreenLeftCorridorCenter();

    state.updateMalachSpawn(0.0f);

    int spawned = 0;
    const float expectedSpawnZ = CrowdControlState::LANE_START_Z + CrowdControl_GetTuning().spawnMargin;
    for (const CrowdControlUnit &m : state.malachim)
    {
        if (!m.active)
            continue;
        ++spawned;
        CHECK(m.pos.x == doctest::Approx(CrowdControlState::ScreenLeftCorridorCenter()));
        CHECK(m.pos.y == doctest::Approx(expectedSpawnZ));
    }
    CHECK(spawned == 1);
}

TEST_CASE("Crowd Control all lanes spawn malachim at the same Z")
{
    const float expectedSpawnZ = CrowdControlState::LANE_START_Z + CrowdControl_GetTuning().spawnMargin;
    const float combatZ = CrowdControlState::malachSpawnZForBase(0.0f, 0, 900);
    const float leftRewardZ = CrowdControlState::malachSpawnZForBase(CrowdControlState::ScreenLeftCorridorCenter(), 0, 900);
    const float rightRewardZ = CrowdControlState::malachSpawnZForBase(CrowdControlState::ScreenRightCorridorCenter(), 0, 900);

    CHECK(combatZ == doctest::Approx(expectedSpawnZ));
    CHECK(leftRewardZ == doctest::Approx(expectedSpawnZ));
    CHECK(rightRewardZ == doctest::Approx(expectedSpawnZ));
}

TEST_CASE("Crowd Control reward lane runners cannot trigger victory")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    state.malachim[0].active = true;
    state.malachim[0].lane = CrowdControlUnitLane::LEFT_REWARD;
    state.malachim[0].speed = tuning.unitSpeed;
    state.malachim[0].pos = glm::vec2(
        CrowdControlState::ScreenLeftCorridorCenter(),
        CrowdControlState::LANE_END_Z + tuning.spawnMargin - 0.01f
    );

    state.updateMovement(0.05f);

    CHECK(state.phase == CrowdControlPhase::RUNNING);
    CHECK(state.endReason == CrowdControlEndReason::NONE);
    CHECK_FALSE(state.malachim[0].active);
}

TEST_CASE("Crowd Control spawned malachim have grace time before fighting")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));

    state.enemies[0].pos = glm::vec2(0.0f, 0.05f);
    state.beginFrontlineDogFights();
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::MOVING);

    state.malachim[0].graceTime = 0.0f;
    state.malachim[0].canFight = true;
    state.beginFrontlineDogFights();
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::FIGHTING);
}

TEST_CASE("Crowd Control fighting and blocking drain TTL like the JS prototype")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));

    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.malachim[0].fightStrength = 0.25f;
    state.malachim[0].hitBuff = 1.0f;
    state.enemies[0].fightStrength = 0.30f;
    state.enemies[0].pos = glm::vec2(0.0f, 0.04f);

    state.updateFights(0.05f);
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.malachim[0].fightStrength == doctest::Approx(0.20f));
    CHECK(state.enemies[0].fightStrength == doctest::Approx(0.25f));

    state.updateFights(0.25f);
    CHECK_FALSE(state.malachim[0].active);
    CHECK_FALSE(state.enemies[0].active);
    CHECK(state.dogsKilled == 1);
}

TEST_CASE("Crowd Control queues SFX for fights, deaths, and boss spawns")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));

    state.sfxEvents.clear();
    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.malachim[0].fightStrength = 0.05f;
    state.enemies[0].fightStrength = 0.05f;
    state.enemies[0].pos = glm::vec2(0.0f, 0.04f);
    state.updateFights(0.05f);

    REQUIRE(state.sfxEvents.count >= 3);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::FIGHT_START);
    REQUIRE(state.particleEvents.count >= 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::FIGHT_CONTACT);
    bool sawAngelDeath = false;
    bool sawEnemyDeath = false;
    bool sawAngelDeathParticle = false;
    bool sawEnemyDeathParticle = false;
    for (int i = 0; i < state.sfxEvents.count; ++i)
    {
        sawAngelDeath |= state.sfxEvents.events[i] == MiniGameSfxEvent::ANGEL_DIED;
        sawEnemyDeath |= state.sfxEvents.events[i] == MiniGameSfxEvent::ENEMY_DIED;
    }
    for (int i = 0; i < state.particleEvents.count; ++i)
    {
        sawAngelDeathParticle |= state.particleEvents.events[i].kind == MiniGameParticleEventKind::ANGEL_DIED;
        sawEnemyDeathParticle |= state.particleEvents.events[i].kind == MiniGameParticleEventKind::ENEMY_DIED;
    }
    CHECK(sawAngelDeath);
    CHECK(sawEnemyDeath);
    CHECK(sawAngelDeathParticle);
    CHECK(sawEnemyDeathParticle);

    state.sfxEvents.clear();
    state.particleEvents.clear();
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::SERAPH));
    REQUIRE(state.sfxEvents.count == 1);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::BOSS_SPAWNED);
    CHECK(state.particleEvents.count == 0);
}

TEST_CASE("Crowd Control hit buff scales direct fight damage")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));

    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.malachim[0].fightStrength = 1.0f;
    state.malachim[0].hitBuff = 2.0f;
    state.enemies[0].fightStrength = 1.0f;
    state.enemies[0].hitBuff = 0.5f;
    state.enemies[0].pos = glm::vec2(0.0f, 0.04f);

    state.updateFights(0.25f);

    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.malachim[0].fightStrength == doctest::Approx(0.875f));
    CHECK(state.enemies[0].fightStrength == doctest::Approx(0.5f));
}

TEST_CASE("Crowd Control boss damage spawns remaining HP floating text")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::SERAPH));

    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.malachim[0].hitBuff = 1.0f;
    state.enemies[0].pos = glm::vec2(0.0f, 0.04f);
    CHECK(state.enemies[0].hp == CrowdControlState::SERAPH_HP);

    state.updateFights(2.50f);

    REQUIRE(state.bossHpTexts[0].active);
    CHECK(std::string(state.bossHpTexts[0].text) == "199");
    CHECK(state.bossHpTexts[0].kind == CrowdControlEnemyKind::SERAPH);
    CHECK(state.bossHpTexts[0].enemyPos.x == doctest::Approx(state.enemies[0].pos.x));

    for (int i = 0; i < 40; ++i)
        state.updateFloatingTexts(0.05f);
    CHECK_FALSE(state.bossHpTexts[0].active);
}

TEST_CASE("Crowd Control dog damage does not spawn boss HP floating text")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));

    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.enemies[0].fightStrength = 1.0f;
    state.enemies[0].pos = glm::vec2(0.0f, 0.04f);

    state.updateFights(0.50f);

    for (const CrowdControlBossHpText &text : state.bossHpTexts)
        CHECK_FALSE(text.active);
}

TEST_CASE("Crowd Control unblocked units move at JS unit speed")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit malach = {};
    malach.active = true;
    malach.speed = CrowdControl_GetTuning().unitSpeed;
    malach.pos = glm::vec2(0.0f, -10.0f);
    state.moveUnit(malach, 1.0f, 1.0f);
    CHECK(malach.pos.y == doctest::Approx(-10.0f + 1.5f));

    CrowdControlUnit dog = {};
    dog.active = true;
    dog.speed = CrowdControl_GetTuning().unitSpeed;
    dog.pos = glm::vec2(0.0f, -10.0f);
    state.moveUnit(dog, 1.0f, -1.0f);
    CHECK(dog.pos.y == doctest::Approx(-10.0f - 1.5f));
}

TEST_CASE("Crowd Control dog combat uses z-front contact instead of strict x overlap")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const float radius = CrowdControl_GetTuning().unitRadius;

    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));
    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.enemies[0].pos = glm::vec2((2.0f * radius) - 0.001f, (2.0f * radius) - 0.001f);

    state.beginFrontlineDogFights();
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::FIGHTING);

    state = CrowdControlState{};
    state.initCrowdControl();
    REQUIRE(state.spawnMalach(glm::vec2(0.0f, 0.0f)));
    REQUIRE(state.spawnEnemy(CrowdControlEnemyKind::DOG));
    state.malachim[0].canFight = true;
    state.malachim[0].graceTime = 0.0f;
    state.enemies[0].pos = glm::vec2((2.0f * radius) + 0.001f, (2.0f * radius) + CrowdControl_GetTuning().frontlineFightDepth);

    state.beginFrontlineDogFights();
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::MOVING);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::MOVING);
}

TEST_CASE("Crowd Control bosses let malachim attack without pairing the boss")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit malach = {};
    malach.active = true;
    malach.canFight = true;
    malach.lane = CrowdControlUnitLane::COMBAT;
    malach.speed = CrowdControl_GetTuning().unitSpeed;
    malach.fightStrength = 10.0f;
    malach.hitBuff = 1.0f;
    malach.pos = glm::vec2(0.0f, -10.0f);
    state.malachim[0] = malach;

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::SERAPH;
    boss.speed = CrowdControl_GetTuning().unitSpeed;
    boss.fightStrength = 10.0f;
    boss.maxFightStrength = 10.0f;
    boss.hp = CrowdControlState::HpFromFightStrength(boss.fightStrength);
    boss.pos = glm::vec2(0.0f, -9.95f);
    state.enemies[0] = boss;

    state.updateFights(0.05f);

    CHECK(state.malachim[0].bossBlocked);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::MOVING);
    CHECK(state.enemies[0].pairedIndex == -1);
    CHECK(state.malachim[0].mode == CrowdControlUnitMode::FIGHTING);
    CHECK(state.malachim[0].pairedIndex == 0);
    CHECK(state.enemies[0].meleeCooldown > 0.0f);
    CHECK(state.enemies[0].fightStrength < 10.0f);
}

TEST_CASE("Crowd Control boss line blocks the whole combat corridor")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    CrowdControlUnit malach = {};
    malach.active = true;
    malach.canFight = true;
    malach.lane = CrowdControlUnitLane::COMBAT;
    malach.speed = tuning.unitSpeed;
    malach.fightStrength = 10.0f;
    malach.pos = glm::vec2(CrowdControlState::MIDDLE_HALF_WIDTH - 0.01f, -10.0f);
    state.malachim[0] = malach;

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::THRONE;
    boss.speed = 0.0f;
    boss.fightStrength = 10.0f;
    boss.maxFightStrength = 10.0f;
    boss.hp = CrowdControlState::HpFromFightStrength(boss.fightStrength);
    boss.pos = glm::vec2(0.0f, -9.96f);
    state.enemies[0] = boss;

    state.updateMovement(0.05f);

    CHECK(state.malachim[0].bossBlocked);
    CHECK(state.malachim[0].pos.y == doctest::Approx(-10.0f));
    CHECK(std::abs(state.malachim[0].pos.x - state.enemies[0].pos.x) > 2.0f * tuning.unitRadius);
}

TEST_CASE("Crowd Control boss smash is cooldown gated while angels keep attacking")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::SERAPH;
    boss.speed = tuning.unitSpeed;
    boss.fightStrength = 20.0f;
    boss.maxFightStrength = 20.0f;
    boss.hp = CrowdControlState::HpFromFightStrength(boss.fightStrength);
    boss.pos = glm::vec2(0.0f, -9.90f);
    state.enemies[0] = boss;

    for (int i = 0; i < 4; ++i)
    {
        CrowdControlUnit malach = {};
        malach.active = true;
        malach.canFight = true;
        malach.lane = CrowdControlUnitLane::COMBAT;
        malach.speed = tuning.unitSpeed;
        malach.fightStrength = 8.0f;
        malach.hitBuff = 1.0f;
        malach.pos = glm::vec2((float)(i - 1) * 0.025f, -9.94f + (float)i * 0.01f);
        state.malachim[i] = malach;
    }

    state.updateFights(0.05f);

    CHECK(state.enemies[0].pairedIndex == -1);
    CHECK(state.enemies[0].fightStrength == doctest::Approx(
        20.0f - 4.0f * 0.05f * tuning.bossIncomingDamageMultiplier
    ));
    int smashed = 0;
    for (int i = 0; i < 4; ++i)
        smashed += state.malachim[i].fightStrength < 4.0f ? 1 : 0;
    CHECK(smashed == CrowdControl_GetTuning().seraphSmashMaxTargets);
    CHECK(state.malachim[0].fightStrength == doctest::Approx(8.0f - tuning.seraphSmashDamage));
    bool sawBossSmashParticles = false;
    for (int i = 0; i < state.particleEvents.count; ++i)
        sawBossSmashParticles |= state.particleEvents.events[i].kind == MiniGameParticleEventKind::BOSS_SMASH;
    CHECK(sawBossSmashParticles);

    const float malachAfterFirstSmash = state.malachim[0].fightStrength;
    const float bossAfterFirstAttack = state.enemies[0].fightStrength;
    state.updateFights(0.05f);

    CHECK(state.enemies[0].fightStrength == doctest::Approx(
        bossAfterFirstAttack - 4.0f * 0.05f * tuning.bossIncomingDamageMultiplier
    ));
    CHECK(state.malachim[0].fightStrength == doctest::Approx(malachAfterFirstSmash));
}

TEST_CASE("Crowd Control bosses hold when no combat malachim exist")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::SERAPH;
    boss.speed = tuning.unitSpeed;
    boss.fightStrength = 10.0f;
    boss.maxFightStrength = 10.0f;
    boss.hp = CrowdControlState::HpFromFightStrength(boss.fightStrength);
    boss.pos = glm::vec2(0.0f, -9.50f);
    state.enemies[0] = boss;

    const float bossBefore = state.enemies[0].pos.y;
    state.updateMovement(0.05f);

    CHECK_FALSE(state.frontmostCombatMalachY(nullptr));
    CHECK(state.enemies[0].pos.y == doctest::Approx(bossBefore));
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::MOVING);
}

TEST_CASE("Crowd Control bosses turn back when malachim pass them")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();

    CrowdControlUnit passedMalach = {};
    passedMalach.active = true;
    passedMalach.canFight = true;
    passedMalach.lane = CrowdControlUnitLane::COMBAT;
    passedMalach.speed = tuning.unitSpeed;
    passedMalach.pos = glm::vec2(0.0f, -9.0f);
    state.malachim[0] = passedMalach;

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::THRONE;
    boss.speed = tuning.unitSpeed;
    boss.fightStrength = 10.0f;
    boss.maxFightStrength = 10.0f;
    boss.hp = CrowdControlState::HpFromFightStrength(boss.fightStrength);
    boss.pos = glm::vec2(0.0f, -9.50f);
    state.enemies[0] = boss;

    const float bossBefore = state.enemies[0].pos.y;
    state.updateMovement(0.05f);

    CHECK(state.bossMovementDirection(state.enemies[0]) == doctest::Approx(1.0f));
    CHECK(state.enemies[0].pos.y > bossBefore);
    CHECK(state.enemies[0].mode == CrowdControlUnitMode::MOVING);
}

TEST_CASE("Crowd Control bosses do not block enemy spawns behind them")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    const CrowdControlTuning tuning = CrowdControl_GetTuning();
    const float spawnZ = CrowdControlState::worldZFromJs(tuning.spawnMargin);

    CrowdControlUnit boss = {};
    boss.active = true;
    boss.kind = CrowdControlEnemyKind::SERAPH;
    boss.pos = glm::vec2(0.0f, spawnZ);
    state.enemies[0] = boss;

    CHECK(state.hasRoomToSpawnEnemy(spawnZ));

    CrowdControlUnit dog = {};
    dog.active = true;
    dog.kind = CrowdControlEnemyKind::DOG;
    dog.pos = glm::vec2(0.0f, spawnZ);
    state.enemies[1] = dog;

    CHECK_FALSE(state.hasRoomToSpawnEnemy(spawnZ));

    state.beginFrontlineDogFights();
    CHECK_FALSE(state.enemies[1].blocked);
}

TEST_CASE("Crowd Control saturated contact keeps consuming instead of clogging forever")
{
    CrowdControlState state = {};
    state.initCrowdControl();
    state.waitingForFirstInput = false;
    for (CrowdControlUnit &m : state.malachim)
        m = CrowdControlUnit{};
    for (CrowdControlUnit &e : state.enemies)
        e = CrowdControlUnit{};

    for (int i = 0; i < 24; ++i)
    {
        state.malachim[i].active = true;
        state.malachim[i].canFight = true;
        state.malachim[i].speed = CrowdControl_GetTuning().unitSpeed;
        state.malachim[i].fightStrength = 0.20f;
        state.malachim[i].maxFightStrength = 0.20f;
        state.malachim[i].pos = glm::vec2(float((i % 6) - 3) * 0.04f, -10.05f + float(i / 6) * 0.03f);

        state.enemies[i].active = true;
        state.enemies[i].kind = CrowdControlEnemyKind::DOG;
        state.enemies[i].speed = CrowdControl_GetTuning().unitSpeed;
        state.enemies[i].fightStrength = 0.20f;
        state.enemies[i].maxFightStrength = 0.20f;
        state.enemies[i].pos = glm::vec2(float((i % 6) - 3) * 0.04f, -9.96f - float(i / 6) * 0.03f);
    }

    const int destroyedBefore = state.destroyedEnemyCount();
    for (int i = 0; i < 20; ++i)
        state.updateCrowdControl(0.05f, 0.0f);
    CHECK(state.destroyedEnemyCount() > destroyedBefore);
}

TEST_CASE("Crowd Control rewards killed dogs and full boss lives")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit dog = {};
    dog.active = true;
    dog.kind = CrowdControlEnemyKind::DOG;
    state.killEnemy(dog);
    CHECK(state.rewardCoins == 1);
    CHECK(state.dogsKilled == 1);

    CrowdControlUnit seraph = {};
    seraph.active = true;
    seraph.kind = CrowdControlEnemyKind::SERAPH;
    state.killEnemy(seraph);
    CHECK(state.rewardCoins == 1 + CrowdControlState::SERAPH_HP);
    CHECK(state.bossHpRewardEarned == CrowdControlState::SERAPH_HP);
    CHECK(state.destroyedEnemyCount() == 2);
}

TEST_CASE("Crowd Control deaths enqueue side-biased fly-out effects")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit malach = {};
    malach.active = true;
    malach.pos = glm::vec2(0.10f, -9.0f);
    malach.lastMove = glm::vec2(0.0f, 0.10f);
    state.spawnDeathFx(malach, true);

    const CrowdControlDeathFx &fx = state.deathFx[0];
    CHECK(fx.active);
    CHECK(fx.malach);
    CHECK(fx.distance >= 1.0f);
    CHECK(fx.distance <= 1.5f);
    CHECK(std::abs(fx.flyDir.x) > std::abs(fx.flyDir.y));

    for (int i = 0; i < 24; ++i)
        state.updateDeathFx(0.05f);
    CHECK_FALSE(state.deathFx[0].active);
}

TEST_CASE("Crowd Control inward death fly-outs get taller arcs")
{
    CHECK(CrowdControlState::DeathMiddleDirectness(0.30f, glm::vec2(-1.0f, 0.0f)) == doctest::Approx(1.0f));
    CHECK(CrowdControlState::DeathMiddleDirectness(0.30f, glm::vec2(1.0f, 0.0f)) == doctest::Approx(0.0f));
    CHECK(CrowdControlState::DeathMiddleDirectness(-0.30f, glm::vec2(0.5f, 0.866f)) == doctest::Approx(0.5f));
    CHECK(CrowdControlState::DeathArcHeightMultiplier(0.30f, glm::vec2(-1.0f, 0.0f)) == doctest::Approx(2.0f));
    CHECK(CrowdControlState::DeathArcHeightMultiplier(0.30f, glm::vec2(1.0f, 0.0f)) == doctest::Approx(1.0f));
    CHECK(CrowdControlState::DeathArcHeightMultiplier(-0.30f, glm::vec2(1.0f, 0.0f)) == doctest::Approx(2.0f));
    CHECK(CrowdControlState::DeathArcHeightMultiplier(-0.30f, glm::vec2(0.5f, 0.866f)) == doctest::Approx(1.5f));
    CHECK(CrowdControlState::DeathArcHeightMultiplier(0.0f, glm::vec2(1.0f, 0.0f)) == doctest::Approx(1.0f));
}

TEST_CASE("Crowd Control boss deaths enqueue spin-only vanish effects")
{
    CrowdControlState state = {};
    state.initCrowdControl();

    CrowdControlUnit seraph = {};
    seraph.active = true;
    seraph.kind = CrowdControlEnemyKind::SERAPH;
    seraph.pos = glm::vec2(-0.15f, -7.0f);
    state.killEnemy(seraph);

    const CrowdControlDeathFx &fx = state.deathFx[0];
    CHECK(fx.active);
    CHECK_FALSE(fx.malach);
    CHECK(fx.kind == CrowdControlEnemyKind::SERAPH);
    CHECK(fx.flyDir.x == doctest::Approx(0.0f));
    CHECK(fx.flyDir.y == doctest::Approx(0.0f));
    CHECK(std::abs(fx.spin) > 0.0f);
}

TEST_CASE("Count Masters gate math clamps and applies choices")
{
    CHECK(CountMastersState::ApplyGateMath(1, {CountMastersOp::ADD, 5}) == 6);
    CHECK(CountMastersState::ApplyGateMath(6, {CountMastersOp::MULTIPLY, 3}) == 18);
    CHECK(CountMastersState::ApplyGateMath(6, {CountMastersOp::SUBTRACT, 10}) == 0);
    CHECK(CountMastersState::ApplyGateMath(63, {CountMastersOp::ADD, 5}) == CountMastersState::MAX_UNITS);
    CHECK(CountMastersState::ApplyGateMath(9, {CountMastersOp::DIVIDE, 2}) == 4);
}

TEST_CASE("Count Masters waits for first input before running")
{
    CountMastersState state = {};
    state.initDefault();
    state.waitingForFirstInput = true;
    const float startZ = state.runnerZ;

    state.tick(1.0f, 0.25f);

    CHECK(state.runnerZ == doctest::Approx(startZ));
    CHECK(state.elapsed == doctest::Approx(0.0f));
    CHECK(state.targetX == doctest::Approx(0.25f));

    state.waitingForFirstInput = false;
    state.tick(0.1f, 0.25f);

    CHECK(state.runnerZ > startZ);
    CHECK(state.elapsed > 0.0f);
}

TEST_CASE("Count Masters seeded gates vary while keeping course layout")
{
    CountMastersState a = {};
    CountMastersState b = {};
    a.initWithSeed(1234u);
    b.initWithSeed(5678u);

    bool anyChoiceDiffers = false;
    for (int i = 0; i < CountMastersState::GATE_COUNT; ++i)
    {
        CHECK(a.gates[i].z == doctest::Approx(b.gates[i].z));
        CHECK(a.gates[i].leftCoinCount >= 1);
        CHECK(a.gates[i].leftCoinCount <= CountMastersState::MAX_GATE_COINS_PER_SIDE);
        CHECK(a.gates[i].rightCoinCount >= 1);
        CHECK(a.gates[i].rightCoinCount <= CountMastersState::MAX_GATE_COINS_PER_SIDE);
        CHECK(b.gates[i].leftCoinCount >= 1);
        CHECK(b.gates[i].leftCoinCount <= CountMastersState::MAX_GATE_COINS_PER_SIDE);
        CHECK(b.gates[i].rightCoinCount >= 1);
        CHECK(b.gates[i].rightCoinCount <= CountMastersState::MAX_GATE_COINS_PER_SIDE);
        anyChoiceDiffers =
            anyChoiceDiffers ||
            a.gates[i].left.op != b.gates[i].left.op ||
            a.gates[i].left.value != b.gates[i].left.value ||
            a.gates[i].right.op != b.gates[i].right.op ||
            a.gates[i].right.value != b.gates[i].right.value;
    }
    CHECK(anyChoiceDiffers);
}

TEST_CASE("Count Masters gate coins are awarded only for the chosen side")
{
    CountMastersState state = {};
    state.initDefault();
    state.waitingForFirstInput = false;
    state.runnerX = -0.20f;
    state.targetX = -0.20f;
    state.runnerZ = state.gates[0].z - 0.001f;

    const int leftCoins = state.gates[0].leftCoinCount;
    const int rightCoins = state.gates[0].rightCoinCount;

    state.tick(0.05f, -0.20f);

    CHECK(state.gates[0].resolved);
    CHECK(state.gates[0].chosenSide == -1);
    CHECK(state.gateCoinsCollected == leftCoins);
    CHECK(state.gateCoinsCollected != leftCoins + rightCoins);
}

TEST_CASE("Count Masters squad combat cancels one for one")
{
    CHECK(CountMastersState::ResolveFight(10, 4) == 6);
    CHECK(CountMastersState::ResolveFight(4, 4) == 0);
    CHECK(CountMastersState::ResolveFight(3, 8) == 0);
    CHECK(CountMastersState::ComputeRewardCoins(0, 0) == 0);
    CHECK(CountMastersState::ComputeRewardCoins(10, 0) == 100);
    CHECK(CountMastersState::ComputeRewardCoins(7, 4) == 74);
}

TEST_CASE("Count Masters circular formation slots start with leader then packed ring")
{
    const glm::vec2 leader = CountMastersState::FormationSlotForUnitIndex(0, 0.12f, -2.0f);
    CHECK(doctest::Approx(leader.x).epsilon(0.001) == 0.12f);
    CHECK(doctest::Approx(leader.y).epsilon(0.001) == -2.0f);

    for (int i = 1; i <= 6; ++i)
    {
        const glm::vec2 slot = CountMastersState::FormationSlotForUnitIndex(i, 0.0f, -2.0f);
        const glm::vec2 d = slot - glm::vec2(0.0f, -2.0f);
        CHECK(doctest::Approx(std::sqrt(d.x * d.x + d.y * d.y)).epsilon(0.001) ==
              CountMastersState::FORMATION_SPACING_X);
    }

    const glm::vec2 firstSecondRing = CountMastersState::FormationSlotForUnitIndex(7, 0.0f, -2.0f);
    const glm::vec2 d = firstSecondRing - glm::vec2(0.0f, -2.0f);
    CHECK(doctest::Approx(std::sqrt(d.x * d.x + d.y * d.y)).epsilon(0.001) ==
          CountMastersState::FORMATION_SPACING_X * 2.0f);
}

TEST_CASE("Count Masters enemies use the same circular formation language")
{
    CountMastersEnemySquad enemy = {};
    enemy.z = -4.0f;
    enemy.count = 7;
    CountMastersState::InitEnemySquadUnits(enemy);

    CHECK(doctest::Approx(enemy.units[0].x).epsilon(0.001) == 0.0f);
    CHECK(doctest::Approx(enemy.units[0].y).epsilon(0.001) == enemy.z);
    for (int i = 1; i <= 6; ++i)
    {
        const glm::vec2 d = enemy.units[i] - enemy.units[0];
        CHECK(doctest::Approx(std::sqrt(d.x * d.x + d.y * d.y)).epsilon(0.001) ==
              CountMastersState::FORMATION_SPACING_X);
    }
}

TEST_CASE("Count Masters followers aim for the opened gate side before moving forward")
{
    CountMastersState state = {};
    state.initDefault();
    state.gates[0].resolved = true;
    state.gates[0].chosenSide = -1;

    const float gateZ = state.gates[0].z;
    const glm::vec2 blockedRightSide(0.25f, gateZ - 0.35f);
    const glm::vec2 formationWantsForward(0.25f, gateZ + 0.35f);
    const glm::vec2 target = state.applyGateFlowTarget(blockedRightSide, formationWantsForward);

    CHECK(target.x < 0.0f);
    CHECK(target.y <= gateZ);
}

TEST_CASE("Count Masters gate rewards appear in formation immediately")
{
    CountMastersState state = {};
    state.initDefault();
    state.runnerX = CountMastersState::GATE_SIDE_CENTER_X;
    state.runnerZ = state.gates[0].z;

    state.syncUnitCount(1, 6);

    CHECK(state.playerCount == 6);
    for (int i = 1; i < state.playerCount; ++i)
    {
        const glm::vec2 slot = CountMastersState::FormationSlotForUnitIndex(i, state.runnerX, state.runnerZ);
        CHECK(state.unitModes[i] == CountMastersUnitMode::Moving);
        CHECK(doctest::Approx(state.units[i].x).epsilon(0.001) == slot.x);
        CHECK(doctest::Approx(state.units[i].y).epsilon(0.001) == slot.y);
    }
}

TEST_CASE("Count Masters fight pairs keep personal spacing")
{
    CountMastersState state = {};
    state.initDefault();
    CountMastersEnemySquad &enemy = state.enemies[0];
    state.playerCount = 2;
    state.units[0] = glm::vec2(0.0f, enemy.z);
    state.units[1] = glm::vec2(-0.02f, enemy.z);
    state.unitModes[0] = CountMastersUnitMode::Moving;
    state.unitModes[1] = CountMastersUnitMode::Moving;
    enemy.units[0] = glm::vec2(0.02f, enemy.z);
    enemy.modes[0] = CountMastersUnitMode::Moving;

    state.beginFightPair(enemy, 1, 0);

    const glm::vec2 d = state.units[1] - enemy.units[0];
    CHECK(doctest::Approx(std::sqrt(d.x * d.x + d.y * d.y)).epsilon(0.001) ==
          CountMastersState::FORMATION_MIN_SEPARATION_M);
    CHECK(doctest::Approx(state.unitFightTime[1]).epsilon(0.001) == 1.0f);
    CHECK(doctest::Approx(enemy.fightTime[0]).epsilon(0.001) == 1.0f);
}

TEST_CASE("Count Masters queues SFX for fight start and paired deaths")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 1;
    state.activeFightSquad = 0;
    state.units[0] = glm::vec2(0.05f, -8.0f);
    state.unitModes[0] = CountMastersUnitMode::Moving;

    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.count = 1;
    enemy.resolved = false;
    enemy.engaged = true;
    enemy.units[0] = glm::vec2(-0.04f, -8.02f);
    enemy.modes[0] = CountMastersUnitMode::Moving;

    state.sfxEvents.clear();
    state.beginFightPair(enemy, 0, 0);
    REQUIRE(state.sfxEvents.count == 1);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::FIGHT_START);
    REQUIRE(state.particleEvents.count == 1);
    CHECK(state.particleEvents.events[0].kind == MiniGameParticleEventKind::FIGHT_CONTACT);

    state.sfxEvents.clear();
    state.particleEvents.clear();
    state.unitFightTime[0] = 0.01f;
    enemy.fightTime[0] = 0.01f;
    state.updateFight(0.02f);

    REQUIRE(state.sfxEvents.count >= 2);
    CHECK(state.sfxEvents.events[0] == MiniGameSfxEvent::ANGEL_DIED);
    CHECK(state.sfxEvents.events[1] == MiniGameSfxEvent::ENEMY_DIED);
    REQUIRE(state.particleEvents.count >= 2);
    bool sawAngelDeathParticle = false;
    bool sawEnemyDeathParticle = false;
    for (int i = 0; i < state.particleEvents.count; ++i)
    {
        sawAngelDeathParticle |= state.particleEvents.events[i].kind == MiniGameParticleEventKind::ANGEL_DIED;
        sawEnemyDeathParticle |= state.particleEvents.events[i].kind == MiniGameParticleEventKind::ENEMY_DIED;
    }
    CHECK(sawAngelDeathParticle);
    CHECK(sawEnemyDeathParticle);
}

TEST_CASE("Count Masters closer valid pair can fight when a farther unit sees enemy first")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 3;
    state.syncUnitCount(1, 3);
    state.activeFightSquad = 0;
    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.engaged = true;
    state.runnerX = enemy.center.x;
    state.runnerZ = enemy.center.y;
    state.units[0] = enemy.center;
    state.units[1] = enemy.units[0] + glm::vec2(-CountMastersState::FORMATION_SPACING_X * 3.0f, 0.0f);
    state.units[2] = enemy.units[0] + glm::vec2(-CountMastersState::FORMATION_MIN_SEPARATION_M * 0.5f, 0.0f);
    enemy.modes[0] = CountMastersUnitMode::Moving;

    state.updateFight(0.016f);

    CHECK(state.unitModes[1] == CountMastersUnitMode::Moving);
    CHECK(state.unitModes[2] == CountMastersUnitMode::Fighting);
    CHECK(enemy.modes[0] == CountMastersUnitMode::Fighting);
}

TEST_CASE("Count Masters does not compact new fighters into dead slots during active melee")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 4;
    state.syncUnitCount(1, 4);
    state.activeFightSquad = 0;
    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.engaged = true;
    state.runnerZ = enemy.z;
    enemy.count = 3;
    CountMastersState::InitEnemySquadUnits(enemy);

    state.units[1] = glm::vec2(-0.05f, enemy.z);
    state.units[2] = glm::vec2(0.05f, enemy.z);
    state.units[3] = glm::vec2(-0.42f, enemy.z);
    enemy.units[0] = glm::vec2(-0.02f, enemy.z);
    enemy.units[1] = glm::vec2(0.08f, enemy.z);
    enemy.units[2] = glm::vec2(0.42f, enemy.z);

    state.beginFightPair(enemy, 1, 0);
    state.beginFightPair(enemy, 2, 1);
    state.unitFightTime[1] = 0.01f;
    enemy.fightTime[0] = 0.01f;
    state.unitFightTime[2] = CountMastersState::FIGHT_DURATION_S;
    enemy.fightTime[1] = CountMastersState::FIGHT_DURATION_S;

    state.updateFight(0.02f);

    CHECK(state.playerCount == 4);
    CHECK(state.unitModes[1] == CountMastersUnitMode::Dead);
    CHECK(state.unitModes[2] == CountMastersUnitMode::Fighting);
    CHECK(state.unitModes[3] == CountMastersUnitMode::Moving);
    CHECK(enemy.count == 3);
    CHECK(enemy.modes[0] == CountMastersUnitMode::Dead);
    CHECK(enemy.modes[1] == CountMastersUnitMode::Fighting);
    CHECK(enemy.modes[2] == CountMastersUnitMode::Moving);
}

TEST_CASE("Count Masters battle keeps leader centered while followers can fight")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 3;
    state.syncUnitCount(1, 3);
    state.activeFightSquad = 0;
    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.engaged = true;
    state.runnerX = enemy.center.x;
    state.runnerZ = enemy.center.y;
    state.units[0] = enemy.center;
    state.units[1] = enemy.center + glm::vec2(-0.10f, 0.0f);
    state.units[2] = enemy.center + glm::vec2(0.10f, 0.0f);

    state.updateFight(0.016f);

    CHECK(state.unitModes[0] == CountMastersUnitMode::Moving);
    CHECK(doctest::Approx(state.units[0].x).epsilon(0.001) == enemy.center.x);
    CHECK(doctest::Approx(state.units[0].y).epsilon(0.001) == enemy.center.y);
}

TEST_CASE("Count Masters engagement keeps non-touching units resting")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 5;
    state.syncUnitCount(1, 5);
    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.count = 4;
    CountMastersState::InitEnemySquadUnits(enemy);

    state.units[1] = glm::vec2(-0.48f, enemy.z + 0.15f);
    state.units[2] = glm::vec2(0.48f, enemy.z + 0.12f);
    state.units[3] = glm::vec2(-0.44f, enemy.z - 0.10f);
    state.units[4] = glm::vec2(0.44f, enemy.z - 0.12f);
    enemy.units[0] = glm::vec2(-0.45f, enemy.z);
    enemy.units[1] = glm::vec2(0.45f, enemy.z);
    enemy.units[2] = glm::vec2(-0.35f, enemy.z - 0.20f);
    enemy.units[3] = glm::vec2(0.35f, enemy.z - 0.20f);

    const glm::vec2 contactCenter(0.06f, enemy.z);
    const glm::vec2 leaderStart = state.units[0];
    const glm::vec2 followerStart = state.units[4];
    state.startEnemyEngagement(0, contactCenter);

    CHECK(state.activeFightSquad == 0);
    CHECK(doctest::Approx(state.runnerX).epsilon(0.001) == contactCenter.x);
    CHECK(doctest::Approx(state.runnerZ).epsilon(0.001) == contactCenter.y);
    CHECK(state.fightDeploymentActive);
    CHECK(state.unitDeployActive[0]);
    CHECK(state.unitDeployActive[4]);
    CHECK(doctest::Approx(state.units[0].x).epsilon(0.001) == leaderStart.x);
    CHECK(doctest::Approx(state.units[0].y).epsilon(0.001) == leaderStart.y);
    CHECK(doctest::Approx(state.units[4].x).epsilon(0.001) == followerStart.x);
    CHECK(doctest::Approx(state.units[4].y).epsilon(0.001) == followerStart.y);

    auto distance = [](glm::vec2 a, glm::vec2 b)
    {
        const glm::vec2 d = a - b;
        return std::sqrt(d.x * d.x + d.y * d.y);
    };
    state.updateFight(CountMastersState::FIGHT_DEPLOY_DURATION_S * 0.5f);
    CHECK(state.fightDeploymentActive);
    CHECK(distance(state.units[0], contactCenter) < distance(leaderStart, contactCenter));
    CHECK(distance(state.units[4], state.unitDeployTarget[4]) < distance(followerStart, state.unitDeployTarget[4]));
    int deployingFighters = 0;
    for (int p = 0; p < state.playerCount; ++p)
        deployingFighters += state.unitModes[p] == CountMastersUnitMode::Fighting ? 1 : 0;
    CHECK(deployingFighters == 0);

    state.updateFight(CountMastersState::FIGHT_DEPLOY_DURATION_S);
    CHECK_FALSE(state.fightDeploymentActive);
    CHECK(distance(state.units[0], contactCenter) < CountMastersState::FORMATION_MIN_SEPARATION_M);

    int fightingPlayers = 0;
    int movingPlayers = 0;
    for (int p = 0; p < state.playerCount; ++p)
    {
        fightingPlayers += state.unitModes[p] == CountMastersUnitMode::Fighting ? 1 : 0;
        movingPlayers += state.unitModes[p] == CountMastersUnitMode::Moving ? 1 : 0;
    }
    CHECK(fightingPlayers + movingPlayers == state.playerCount);
    CHECK(state.unitModes[0] == CountMastersUnitMode::Moving);
}

TEST_CASE("Count Masters first contact can be a follower, not only the leader")
{
    CountMastersState state = {};
    state.initDefault();
    state.gates[0].resolved = true;
    state.gates[0].chosenSide = 1;
    state.playerCount = 3;
    state.syncUnitCount(1, 3);
    CountMastersEnemySquad &enemy = state.enemies[0];
    state.runnerX = 0.0f;
    state.runnerZ = enemy.z + 0.35f;
    state.units[0] = glm::vec2(state.runnerX, state.runnerZ);
    state.units[1] = enemy.units[0] + glm::vec2(-CountMastersState::FORMATION_MIN_SEPARATION_M * 0.5f, 0.0f);
    state.units[2] = glm::vec2(0.25f, state.runnerZ);
    const float expectedContactX = (state.units[1].x + enemy.units[0].x) * 0.5f;

    state.tick(0.016f, 0.0f);

    CHECK(state.activeFightSquad == 0);
    CHECK(state.enemies[0].engaged);
    CHECK(state.enemies[0].leaderReachedCenter);
    CHECK(doctest::Approx(state.runnerX).epsilon(0.05f) == expectedContactX);
}

TEST_CASE("Count Masters fight keeps Malachs and cherubs on their own sides")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 5;
    state.syncUnitCount(1, 5);
    state.runnerX = 0.0f;
    state.runnerZ = -4.0f;
    state.units[0] = glm::vec2(state.runnerX, state.runnerZ);

    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.count = 5;
    CountMastersState::InitEnemySquadUnits(enemy);

    std::array<glm::vec2, CountMastersState::MAX_UNITS> playerTargets{};
    std::array<glm::vec2, 64> enemyTargets{};
    state.assignSharedFightSlots(enemy, playerTargets, enemyTargets);

    auto dist = [](glm::vec2 a, glm::vec2 b)
    {
        const glm::vec2 d = a - b;
        return std::sqrt(d.x * d.x + d.y * d.y);
    };

    CHECK(dist(playerTargets[0], glm::vec2(state.runnerX, state.runnerZ)) < 0.001f);
    for (int p = 1; p < state.playerCount; ++p)
    {
        CHECK(playerTargets[p].y <= state.runnerZ + 0.001f);
        for (int e = 0; e < enemy.count; ++e)
            CHECK(dist(playerTargets[p], enemyTargets[e]) > 0.001f);
    }
    for (int e = 0; e < enemy.count; ++e)
        CHECK(enemyTargets[e].y > state.runnerZ + 0.001f);
}

TEST_CASE("Count Masters leader contact chooses a gate side and spawns glass shards")
{
    CountMastersState state = {};
    state.initDefault();
    state.runnerX = CountMastersState::GATE_SIDE_CENTER_X;
    state.targetX = CountMastersState::GATE_SIDE_CENTER_X;
    state.runnerZ = state.gates[0].z + 0.001f;

    state.tick(0.016f, CountMastersState::GATE_SIDE_CENTER_X);

    CHECK(state.gates[0].resolved);
    CHECK(state.gates[0].chosenSide == 1);
    int activeShardCount = 0;
    for (const CountMastersGateShard &shard : state.gateShards)
        activeShardCount += shard.active ? 1 : 0;
    CHECK(activeShardCount == CountMastersState::SHARDS_PER_GATE);
}

TEST_CASE("Count Masters enemy contact enters timed fight before removing paired pieces")
{
    CountMastersState state = {};
    state.initDefault();
    state.gates[0].resolved = true;
    state.gates[0].chosenSide = 1;
    state.playerCount = 3;
    state.syncUnitCount(1, 3);
    state.runnerX = 0.0f;
    state.targetX = 0.0f;
    state.runnerZ = state.enemies[0].z + 0.35f;
    state.units[0] = glm::vec2(state.runnerX, state.runnerZ);
    state.units[1] = state.enemies[0].units[0] + glm::vec2(-CountMastersState::FORMATION_MIN_SEPARATION_M * 0.5f, 0.0f);
    state.units[2] = glm::vec2(0.10f, state.runnerZ + 0.03f);

    state.tick(0.016f, 0.0f);

    CHECK(state.activeFightSquad == 0);
    CHECK_FALSE(state.enemies[0].resolved);
    CHECK(state.playerCount == 3);
    CHECK(state.enemies[0].count == 2);

    for (int i = 0; i < 360 && state.activeFightSquad >= 0; ++i)
        state.tick(0.016f, 0.0f);

    CHECK(state.enemies[0].resolved);
    CHECK(state.activeFightSquad == -1);
    CHECK(state.playerCount == 1);
    CHECK(state.enemies[0].count == 0);
}

TEST_CASE("Count Masters fight deaths enqueue malach and cherub fly-out effects")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 1;
    state.activeFightSquad = 0;
    state.units[0] = glm::vec2(0.05f, -8.0f);
    state.unitModes[0] = CountMastersUnitMode::Fighting;
    state.unitTargetEnemy[0] = 0;
    state.unitFightTime[0] = 0.01f;

    CountMastersEnemySquad &enemy = state.enemies[0];
    enemy.count = 1;
    enemy.resolved = false;
    enemy.engaged = true;
    enemy.units[0] = glm::vec2(-0.04f, -8.02f);
    enemy.modes[0] = CountMastersUnitMode::Fighting;
    enemy.targetPlayer[0] = 0;
    enemy.fightTime[0] = 0.01f;

    state.updateFight(0.02f);

    CHECK(state.phase == CountMastersPhase::LOST);
    CHECK(state.deathFx[0].active);
    CHECK(state.deathFx[0].malach);
    CHECK(state.deathFx[0].delay >= 0.0f);
    CHECK(state.deathFx[0].delay <= 0.4f);
    CHECK(state.deathFx[1].active);
    CHECK_FALSE(state.deathFx[1].malach);
    CHECK(state.deathFx[1].delay >= 0.0f);
    CHECK(state.deathFx[1].delay <= 0.4f);
    CHECK(std::abs(state.deathFx[0].flyDir.x) > std::abs(state.deathFx[0].flyDir.y));
    CHECK(std::abs(state.deathFx[1].flyDir.x) > std::abs(state.deathFx[1].flyDir.y));
}

TEST_CASE("Count Masters death fly-outs wait through random delay before aging")
{
    CountMastersState state = {};
    state.initDefault();
    state.spawnDeathFx(glm::vec2(0.2f, -8.0f), true, glm::vec2(0.0f, CountMastersState::RUN_SPEED_MPS));

    REQUIRE(state.deathFx[0].active);
    state.deathFx[0].delay = 0.20f;
    state.updateDeathFx(0.05f);
    CHECK(state.deathFx[0].delay == doctest::Approx(0.15f));
    CHECK(state.deathFx[0].age == doctest::Approx(0.0f));

    for (int i = 0; i < 3; ++i)
        state.updateDeathFx(0.05f);
    CHECK(state.deathFx[0].delay == doctest::Approx(0.0f));
    CHECK(state.deathFx[0].age == doctest::Approx(0.0f));

    state.updateDeathFx(0.05f);
    CHECK(state.deathFx[0].age == doctest::Approx(0.0f));

    state.updateDeathFx(0.05f);
    CHECK(state.deathFx[0].age == doctest::Approx(0.05f));
}

TEST_CASE("Count Masters elects a new leader when current leader is busy fighting")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 3;
    state.units[0] = glm::vec2(-0.20f, -4.0f);
    state.units[1] = glm::vec2(0.15f, -4.0f);
    state.units[2] = glm::vec2(0.25f, -4.0f);
    state.unitModes[0] = CountMastersUnitMode::Fighting;
    state.unitModes[1] = CountMastersUnitMode::Moving;
    state.unitModes[2] = CountMastersUnitMode::Moving;

    state.electMovingLeaderIfNeeded();

    CHECK(state.unitModes[0] == CountMastersUnitMode::Moving);
    CHECK(doctest::Approx(state.units[0].x).epsilon(0.001) == 0.15f);
    CHECK(doctest::Approx(state.runnerX).epsilon(0.001) == state.units[0].x);
    CHECK(doctest::Approx(state.runnerZ).epsilon(0.001) == state.units[0].y);
}

TEST_CASE("Count Masters pin crash captures direction and scores pins plus standers")
{
    CountMastersState state = {};
    state.initDefault();
    state.playerCount = 10;
    state.syncUnitCount(1, 10);
    state.runnerX = 0.0f;
    state.runnerZ = CountMastersState::PIN_RACK_FRONT_Z - 0.02f;
    for (int i = 0; i < state.playerCount; ++i)
    {
        state.units[i] = glm::vec2(CountMastersState::PinPositionForIndex(i).x, state.runnerZ);
        state.unitModes[i] = CountMastersUnitMode::Moving;
    }
    state.recordMotionHistory();

    REQUIRE(state.anyMovingPlayerTouchesStandingPin());
    state.beginPinCrash();
    CHECK(state.phase == CountMastersPhase::PIN_CRASH);
    CHECK(state.pinCrashNeedsPhysicsStart);
    state.markPinCrashPhysicsStarted();

    std::array<glm::vec2, CountMastersState::MAX_UNITS> livePositions{};
    for (int i = 0; i < state.playerCount; ++i)
        livePositions[i] = state.units[i] + glm::vec2(0.0f, 0.20f);
    state.syncPinCrashFromPhysics(CountMastersState::PIN_COUNT, state.playerCount, livePositions.data(), 0.016f);
    CHECK(state.phase == CountMastersPhase::PIN_CRASH);
    for (int i = 0; i < 80 && state.phase == CountMastersPhase::PIN_CRASH; ++i)
        state.syncPinCrashFromPhysics(CountMastersState::PIN_COUNT, state.playerCount, livePositions.data(), 0.016f);

    CHECK(state.phase == CountMastersPhase::WON);
    CHECK(state.pinsHit == CountMastersState::PIN_COUNT);
    CHECK(state.standers > 0);
    CHECK(state.rewardCoins == CountMastersState::PIN_COUNT * 10 + state.standers);
}

TEST_CASE("Count Masters target pins sit near lane end and on lane surface")
{
    CHECK(CountMastersState::PIN_CENTER_Y == doctest::Approx(0.19f));

    const glm::vec2 headPin = CountMastersState::PinPositionForIndex(0);
    const glm::vec2 backLeftPin = CountMastersState::PinPositionForIndex(6);
    const glm::vec2 backRightPin = CountMastersState::PinPositionForIndex(9);

    CHECK(headPin.y == doctest::Approx(CountMastersState::PIN_RACK_FRONT_Z));
    CHECK(backLeftPin.y == doctest::Approx(CountMastersState::PIN_RACK_BACK_Z));
    CHECK(backRightPin.y == doctest::Approx(CountMastersState::PIN_RACK_BACK_Z));
    CHECK(CountMastersState::PIN_RACK_BACK_Z == doctest::Approx(0.75f));
    CHECK((CountMastersState::PIN_RACK_BACK_Z - CountMastersState::PIN_RACK_FRONT_Z) < 0.90f);
}

TEST_CASE("Count Masters first pin touch waits for Jolt handoff")
{
    CountMastersState state = {};
    state.initDefault();
    for (CountMastersGateRow &gate : state.gates)
        gate.resolved = true;
    for (CountMastersEnemySquad &enemy : state.enemies)
    {
        enemy.resolved = true;
        enemy.count = 0;
    }

    const glm::vec2 headPin = CountMastersState::PinPositionForIndex(0);
    state.runnerX = headPin.x;
    state.targetX = headPin.x;
    state.runnerZ = headPin.y - (CountMastersState::PIN_MEMBER_RADIUS_M + CountMastersState::PIN_RADIUS_M) * 0.5f;
    state.units[0] = glm::vec2(state.runnerX, state.runnerZ);
    state.unitModes[0] = CountMastersUnitMode::Moving;
    state.recordMotionHistory();

    state.tick(0.016f, state.runnerX);

    CHECK(state.phase == CountMastersPhase::PIN_CRASH);
    CHECK(state.pinCrashNeedsPhysicsStart);
    CHECK(!state.pinCrashPhysicsStarted);
    CHECK(!state.pinCrashScoringComplete);
    CHECK(state.pinsHit == 0);
    CHECK(state.standingPinCount() == CountMastersState::PIN_COUNT);
}

TEST_CASE("Count Masters default course can be won by choosing strong gates")
{
    CountMastersState state = {};
    state.initDefault();

    REQUIRE(state.phase == CountMastersPhase::RUNNING);
    CHECK(state.playerCount == 1);
    CHECK(state.gates[0].left.value == 5);
    CHECK(state.gates[0].right.op == CountMastersOp::MULTIPLY);

    std::array<glm::vec2, CountMastersState::MAX_UNITS> livePositions{};
    for (int i = 0; i < 4000 && !state.isDone(); ++i)
    {
        state.tick(0.016f, CountMastersState::LANE_HALF_WIDTH);
        if (state.phase == CountMastersPhase::PIN_CRASH)
        {
            if (state.pinCrashNeedsPhysicsStart)
                state.markPinCrashPhysicsStarted();
            for (int p = 0; p < state.playerCount; ++p)
                livePositions[p] = state.units[p];
            state.syncPinCrashFromPhysics(CountMastersState::PIN_COUNT, state.playerCount, livePositions.data(), 0.016f);
        }
    }

    CHECK(state.phase == CountMastersPhase::WON);
    CHECK(state.playerCount > 0);
    CHECK(state.pinsHit > 0);
    CHECK(state.rewardCoins > 0);
}
