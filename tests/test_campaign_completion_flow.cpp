#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_completion_flow.h"

TEST_CASE("Campaign completion flow keeps finished campaigns out of ordinary level replay")
{
    CHECK(
        Campaign_ResumeFlowForState(false, false) == CampaignResumeFlow::CurrentLevel
    );
    CHECK(
        Campaign_ResumeFlowForState(true, false) == CampaignResumeFlow::CompletedSummary
    );
    CHECK(
        Campaign_ResumeFlowForState(true, true) == CampaignResumeFlow::PostgameFreeplay
    );
    CHECK(
        Campaign_ResumeFlowForState(false, true) == CampaignResumeFlow::PostgameFreeplay
    );
}
