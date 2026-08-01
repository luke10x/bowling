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

TEST_CASE("Coin fly animations can be emitted after a short delay")
{
    CoinLane lane {};

    CHECK(lane.spawnFlyAnimation(
        {0.0f, 0.0f},
        {10.0f, 10.0f},
        CollectableVisualKind::Coin,
        true,
        30.0f,
        0.20f,
        true
    ));

    REQUIRE(lane.flyAnimations[0].active);
    CHECK_FALSE(lane.flyAnimations[0].isVisible());

    int startedSfx = 0;
    CHECK(lane.updateFlyAnimations(0.10f) == 0);
    CHECK(startedSfx == 0);
    CHECK_FALSE(lane.flyAnimations[0].isVisible());

    CHECK(lane.updateFlyAnimations(0.10f, &startedSfx) == 0);
    CHECK(startedSfx == 1);
    CHECK(lane.flyAnimations[0].isVisible());
    CHECK(lane.flyAnimations[0].currentPos.x == doctest::Approx(0.0f));

    CHECK(lane.updateFlyAnimations(CoinFlyConfig::FLY_DURATION, &startedSfx) == 1);
    CHECK(startedSfx == 1);
    CHECK(lane.getActiveFlyCount() == 0);
}
