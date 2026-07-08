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
    CHECK(CountMastersState::ComputeRewardCoins(0) == 0);
    CHECK(CountMastersState::ComputeRewardCoins(10) == 40);
    CHECK(CountMastersState::ComputeRewardCoins(13) == 46);
}

TEST_CASE("Count Masters triangular formation slots start with leader then two-unit row")
{
    const glm::vec2 leader = CountMastersState::FormationSlotForUnitIndex(0, 0.12f, -2.0f);
    CHECK(doctest::Approx(leader.x).epsilon(0.001) == 0.12f);
    CHECK(doctest::Approx(leader.y).epsilon(0.001) == -2.0f);

    const glm::vec2 leftSecondRow = CountMastersState::FormationSlotForUnitIndex(1, 0.0f, -2.0f);
    const glm::vec2 rightSecondRow = CountMastersState::FormationSlotForUnitIndex(2, 0.0f, -2.0f);
    CHECK(leftSecondRow.y > leader.y);
    CHECK(doctest::Approx(leftSecondRow.x).epsilon(0.001) == -CountMastersState::FORMATION_SPACING_X * 0.5f);
    CHECK(doctest::Approx(rightSecondRow.x).epsilon(0.001) == CountMastersState::FORMATION_SPACING_X * 0.5f);
}

TEST_CASE("Count Masters followers aim for the opened gate side before moving forward")
{
    CountMastersState state = {};
    state.initDefault();
    state.gates[0].resolved = true;
    state.gates[0].chosenSide = -1;

    const float gateZ = state.gates[0].z;
    const glm::vec2 blockedRightSide(0.25f, gateZ + 0.35f);
    const glm::vec2 formationWantsForward(0.25f, gateZ - 0.35f);
    const glm::vec2 target = state.applyGateFlowTarget(blockedRightSide, formationWantsForward);

    CHECK(target.x < 0.0f);
    CHECK(target.y >= gateZ);
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

TEST_CASE("Count Masters default course can be won by choosing strong gates")
{
    CountMastersState state = {};
    state.initDefault();

    REQUIRE(state.phase == CountMastersPhase::RUNNING);
    CHECK(state.playerCount == 1);
    CHECK(state.gates[0].left.value == 5);
    CHECK(state.gates[0].right.op == CountMastersOp::MULTIPLY);

    for (int i = 0; i < 4000 && state.phase == CountMastersPhase::RUNNING; ++i)
        state.tick(0.016f, CountMastersState::LANE_HALF_WIDTH);

    CHECK(state.phase == CountMastersPhase::WON);
    CHECK(state.playerCount > 0);
    CHECK(state.pinsHit > 0);
    CHECK(state.rewardCoins > 0);
}
