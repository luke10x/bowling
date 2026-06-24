#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_enemy_block_timing.h"

TEST_CASE("Enemy auto block timing waits until the ball is within the warning window")
{
    const CampaignEnemyBlockTimingPlan plan =
        CampaignEnemyBlockMakeTimingPlan(1.25f, -18.0f, -12.0f, 10.0f, 1.0f);

    REQUIRE(plan.valid);
    CHECK(plan.forwardDistanceM == doctest::Approx(6.0f));
    CHECK(plan.etaS == doctest::Approx(0.6f));
    CHECK(plan.deployAtS == doctest::Approx(1.45f));
}

TEST_CASE("Enemy auto block timing supports reverse lane direction")
{
    const CampaignEnemyBlockTimingPlan plan =
        CampaignEnemyBlockMakeTimingPlan(2.0f, 2.0f, -4.0f, -12.0f, -1.0f);

    REQUIRE(plan.valid);
    CHECK(plan.forwardDistanceM == doctest::Approx(6.0f));
    CHECK(plan.etaS == doctest::Approx(0.5f));
    CHECK(plan.deployAtS == doctest::Approx(2.1f));
}

TEST_CASE("Enemy auto block plan can be dodged if NOS makes the ball arrive early")
{
    const CampaignEnemyBlockTimingPlan plan =
        CampaignEnemyBlockMakeTimingPlan(0.0f, -18.0f, -10.0f, 10.0f, 1.0f);

    REQUIRE(plan.valid);
    CHECK(plan.deployAtS == doctest::Approx(0.4f));

    const float acceleratedArrivalS = 0.25f;
    const float ballZAtArrival = -10.0f;

    CHECK(acceleratedArrivalS < plan.deployAtS);
    CHECK(CampaignEnemyBlockForwardDistanceM(ballZAtArrival, -10.0f, 1.0f) <= 0.0f);
}
