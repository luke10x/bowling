#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../minigames/coin_rush/coin_rush.h"
#include "../minigames/count_masters/count_masters.h"

TEST_CASE("Coin rush minigame lays out a centered full-lane coin grid")
{
    CoinLane lane = {};
    MiniGameCoinRush::InitCoinGrid(&lane);

    REQUIRE(lane.activeCount == MiniGameCoinRush::ComputeCoinCount());
    REQUIRE(lane.activeCount > 100);
    CHECK(lane.currentPattern == CoinPattern::Static);
    CHECK(lane.visualKind == CollectableVisualKind::Coin);
    CHECK(lane.deployedGemCount == 0);

    const int columns = MiniGameCoinRush::ComputeColumnCount();
    const int rows = MiniGameCoinRush::ComputeRowCount();
    REQUIRE(columns >= 2);
    REQUIRE(rows >= 2);

    const Coin &first = lane.coins[0];
    const Coin &second = lane.coins[1];
    const Coin &nextRow = lane.coins[columns];
    const Coin &last = lane.coins[lane.activeCount - 1];

    CHECK(first.state == CoinState::Active);
    CHECK(first.visualKind == CollectableVisualKind::Coin);
    CHECK(doctest::Approx(second.position.x - first.position.x).epsilon(0.001) == MiniGameCoinRush::GRID_SPACING_M);
    CHECK(doctest::Approx(nextRow.position.z - first.position.z).epsilon(0.001) == MiniGameCoinRush::GRID_SPACING_M);
    CHECK(doctest::Approx(first.position.x + last.position.x).epsilon(0.001) == 0.0f);
    CHECK(first.position.z >= CoinLane::LANE_START_Z);
    CHECK(last.position.z <= CoinLane::LANE_END_Z);
}

TEST_CASE("Count Masters gate math clamps and applies choices")
{
    CHECK(CountMastersState::ApplyGateMath(1, {CountMastersOp::ADD, 5}) == 6);
    CHECK(CountMastersState::ApplyGateMath(6, {CountMastersOp::MULTIPLY, 3}) == 18);
    CHECK(CountMastersState::ApplyGateMath(6, {CountMastersOp::SUBTRACT, 10}) == 0);
    CHECK(CountMastersState::ApplyGateMath(63, {CountMastersOp::ADD, 5}) == CountMastersState::MAX_UNITS);
    CHECK(CountMastersState::ApplyGateMath(9, {CountMastersOp::DIVIDE, 2}) == 4);
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
