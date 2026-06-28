#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../coins.h"

TEST_CASE("Coin fly animations only award player bank when explicitly flagged")
{
    CoinLane lane {};

    CHECK(lane.spawnFlyAnimation({0.0f, 0.0f}, {10.0f, 10.0f}, CollectableVisualKind::Coin, true, 30.0f));
    CHECK(lane.spawnFlyAnimation({0.0f, 0.0f}, {10.0f, 10.0f}, CollectableVisualKind::Coin, false, 90.0f));
    CHECK(lane.spawnFlyAnimation({0.0f, 0.0f}, {10.0f, 10.0f}, CollectableVisualKind::Gem, false, 90.0f));

    int earned = 0;
    for (int i = 0; i < 4; ++i)
        earned += lane.updateFlyAnimations(0.25f);

    CHECK(earned == 1);
    CHECK(lane.getActiveFlyCount() == 0);
}
