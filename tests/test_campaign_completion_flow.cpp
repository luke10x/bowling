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

TEST_CASE("First Level 13 clear celebrates before its normal results screen")
{
    CHECK(Campaign_FinalResultFlowForState(true, false, 13, 13, false) ==
          CampaignFinaleFlow::CelebrationThenResult);
    CHECK(Campaign_FinalResultFlowForState(true, false, 13, 13, true) ==
          CampaignFinaleFlow::NormalResult);
    CHECK(Campaign_FinalResultFlowForState(false, false, 13, 13, false) ==
          CampaignFinaleFlow::NormalResult);
    CHECK(Campaign_FinalResultFlowForState(true, true, 13, 13, false) ==
          CampaignFinaleFlow::NormalResult);
}

TEST_CASE("First-clear finale keeps results between celebration and Angel greeting")
{
    CHECK(Campaign_FinaleCloseFlowForState(true) == CampaignFinaleFlow::NormalResult);
    CHECK(Campaign_ResultDismissFlowForState(true) == CampaignFinaleFlow::AngelGreeting);
    CHECK(Campaign_FinaleCloseFlowForState(false) == CampaignFinaleFlow::AngelGreeting);
    CHECK(Campaign_ResultDismissFlowForState(false) == CampaignFinaleFlow::NoTransition);
}

TEST_CASE("Queued bonus keeps the completed level biome visible until the bonus starts")
{
    CHECK(Campaign_VisualBiomeSourceForState(/*bonusQueued=*/true, /*bonusActive=*/false) ==
          CampaignVisualBiomeSource::BonusSource);
    CHECK(Campaign_VisualBiomeSourceForState(/*bonusQueued=*/false, /*bonusActive=*/true) ==
          CampaignVisualBiomeSource::BonusSource);
    CHECK(Campaign_VisualBiomeSourceForState(/*bonusQueued=*/false, /*bonusActive=*/false) ==
          CampaignVisualBiomeSource::CurrentCampaignLevel);
}

TEST_CASE("A configured campaign bonus is offered only once per non-final level")
{
    CHECK(Campaign_ShouldOfferLevelBonus(2, 13, /*configured=*/true, /*alreadyGranted=*/false));
    CHECK_FALSE(Campaign_ShouldOfferLevelBonus(2, 13, /*configured=*/true, /*alreadyGranted=*/true));
    CHECK_FALSE(Campaign_ShouldOfferLevelBonus(13, 13, /*configured=*/true, /*alreadyGranted=*/false));
    CHECK_FALSE(Campaign_ShouldOfferLevelBonus(2, 13, /*configured=*/false, /*alreadyGranted=*/false));
}
