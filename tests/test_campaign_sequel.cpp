#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"
#include "../campaign_sequel.h"

TEST_CASE("Sequel keeps five recent games and five leaders per settings combination")
{
    CampaignSequelHistory history;
    for (int i = 0; i < 12; ++i)
        CampaignSequel_Record(history, {100 + i, 90, 1000 + i, (unsigned char)(i % 2)});
    CHECK(history.recent.count == 5);
    CHECK(history.recent.games[0].score == 111);
    CHECK(history.recent.games[4].score == 107);
    CHECK(history.top[0].count == 5);
    CHECK(history.top[1].count == 5);
    CHECK(history.top[0].games[0].score == 110);
    CHECK(history.top[1].games[0].score == 111);
    const auto combined = CampaignSequel_Top(history, false, 0);
    CHECK(combined.count == 5);
    CHECK(combined.games[0].score == 111);
    CHECK(combined.games[4].score == 107);
    const auto filtered = CampaignSequel_Top(history, true, 0);
    CHECK(filtered.games[0].score == 110);
}

TEST_CASE("Sequel list survives storage encoding and chart bounds cover every game")
{
    CampaignSequelHistory history;
    CampaignSequel_Record(history, {80, 111, 1000, 0});
    CampaignSequel_Record(history, {114, 100, 2000, 7});
    const auto loaded = CampaignSequel_Decode(CampaignSequel_Encode(history.recent));
    CHECK(loaded.count == 2);
    CHECK(loaded.games[0].score == 114);
    CHECK(loaded.games[1].settings == 0);
    int floor = -1, ceiling = -1;
    CampaignSequel_ChartRange(loaded, floor, ceiling);
    CHECK(floor <= 80);
    CHECK(ceiling >= 114);
    CHECK(ceiling > floor);
}
