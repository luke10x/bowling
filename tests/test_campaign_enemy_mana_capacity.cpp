#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_enemy_mana_capacity.h"

TEST_CASE("Enemy mana capacity scales by avatar tier")
{
    CHECK(CampaignEnemyManaCapacityScaleForAvatar(0) == doctest::Approx(1.0f));
    CHECK(CampaignEnemyManaCapacityScaleForAvatar(1) == doctest::Approx(2.0f));
    CHECK(CampaignEnemyManaCapacityScaleForAvatar(2) == doctest::Approx(3.0f));
    CHECK(CampaignEnemyManaCapacityScaleForAvatar(3) == doctest::Approx(4.0f));
    CHECK(CampaignEnemyManaCapacityScaleForAvatar(99) == doctest::Approx(1.0f));
}

