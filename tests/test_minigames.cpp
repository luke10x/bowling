#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../minigames/coin_rush/coin_rush.h"

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
