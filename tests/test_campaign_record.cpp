#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"
#include "../campaign_record.h"

TEST_CASE("Campaign starts only after a first roll")
{
    CHECK_FALSE(CampaignRecord_HasStartedRoll(0));
    CHECK(CampaignRecord_HasStartedRoll(1));
}

TEST_CASE("Campaign quits are started minus losses minus wins")
{
    CHECK(CampaignRecord_Quits(10, 3, 4) == 3);
    CHECK(CampaignRecord_Quits(1, 0, 1) == 0);
    CHECK(CampaignRecord_Quits(0, 0, 0) == 0);
}
