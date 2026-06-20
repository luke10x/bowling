#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../ball_render.h"

TEST_CASE("Ball render atlas region maps first and later balls to distinct tiles")
{
    const int catalogCount = 35;

    const BallAtlasRegion first = BallRender_AtlasRegionForId(0, catalogCount);
    const BallAtlasRegion second = BallRender_AtlasRegionForId(1, catalogCount);
    const BallAtlasRegion seventeenth = BallRender_AtlasRegionForId(16, catalogCount);

    CHECK(first.startX == doctest::Approx(1.0f));
    CHECK(first.startY == doctest::Approx(1.0f));

    CHECK(second.startX == doctest::Approx(1.0f));
    CHECK(second.startY == doctest::Approx(1.0f + 1.0f / 16.0f));

    CHECK(seventeenth.startX == doctest::Approx(1.0f + 2.0f / 16.0f));
    CHECK(seventeenth.startY == doctest::Approx(1.0f));

    CHECK(second.startY != doctest::Approx(first.startY));
    CHECK(seventeenth.startX != doctest::Approx(first.startX));
}

TEST_CASE("Ball render atlas region clamps invalid ids")
{
    const int catalogCount = 35;

    const BallAtlasRegion negative = BallRender_AtlasRegionForId(-3, catalogCount);
    const BallAtlasRegion first = BallRender_AtlasRegionForId(0, catalogCount);
    const BallAtlasRegion huge = BallRender_AtlasRegionForId(999, catalogCount);
    const BallAtlasRegion last = BallRender_AtlasRegionForId(catalogCount - 1, catalogCount);

    CHECK(negative.startX == doctest::Approx(first.startX));
    CHECK(negative.startY == doctest::Approx(first.startY));
    CHECK(huge.startX == doctest::Approx(last.startX));
    CHECK(huge.startY == doctest::Approx(last.startY));
}

TEST_CASE("Ball render selection uses player ball for player turn and enemy ball for enemy turn")
{
    const int catalogCount = 35;

    CHECK(BallRender_SelectBallIdForTurn(27, 8, false, false, catalogCount) == 27);
    CHECK(BallRender_SelectBallIdForTurn(27, 8, true, false, catalogCount) == 27);
    CHECK(BallRender_SelectBallIdForTurn(27, 8, true, true, catalogCount) == 8);
}

TEST_CASE("Ball render selection clamps chosen player and enemy ids")
{
    const int catalogCount = 35;

    CHECK(BallRender_SelectBallIdForTurn(-1, 8, false, false, catalogCount) == 0);
    CHECK(BallRender_SelectBallIdForTurn(27, 999, true, true, catalogCount) == catalogCount - 1);
}
