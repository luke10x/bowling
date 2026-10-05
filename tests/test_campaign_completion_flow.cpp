#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <string>

#include "../campaign_completion_flow.h"
#include "../campaign_time_format.h"

TEST_CASE("Campaign completion flow sends finished campaigns to post-campaign play")
{
    CHECK(
        Campaign_ResumeFlowForState(false, false) == CampaignResumeFlow::CurrentLevel
    );
    CHECK(
        Campaign_ResumeFlowForState(true, false) == CampaignResumeFlow::PostgameFreeplay
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

TEST_CASE("Campaign reset preserves inventory while factory reset does not")
{
    CHECK(CampaignReset_PreservesBallInventory(CampaignResetScope::CampaignOnly));
    CHECK_FALSE(CampaignReset_PreservesBallInventory(CampaignResetScope::Factory));
}

TEST_CASE("Campaign time-to-beat formatting introduces hours after 59 minutes")
{
    char time[24] = {};
    Campaign_FormatTimeToBeat(59 * 60 + 7, time, sizeof(time));
    CHECK(std::string(time) == "59:07");
    Campaign_FormatTimeToBeat(60 * 60, time, sizeof(time));
    CHECK(std::string(time) == "1:00:00");
    Campaign_FormatTimeToBeat(2 * 60 * 60 + 3 * 60 + 4, time, sizeof(time));
    CHECK(std::string(time) == "2:03:04");
}

TEST_CASE("Result actions distinguish campaign and random levels")
{
    CHECK(Campaign_ResultActionsForState(true, true, true).repeat == false);
    CHECK(Campaign_ResultActionsForState(true, true, true).next == true);
    CHECK(Campaign_ResultActionsForState(true, true, false).repeat == true);
    CHECK(Campaign_ResultActionsForState(true, true, false).next == true);
    CHECK(Campaign_ResultActionsForState(true, false, false).repeat == true);
    CHECK(Campaign_ResultActionsForState(true, false, false).next == false);
    CHECK(Campaign_ResultActionsForState(false, true, false).repeat == false);
    CHECK(Campaign_ResultActionsForState(false, true, false).next == true);
    CHECK(Campaign_ResultActionsForState(false, false, false).repeat == false);
    CHECK(Campaign_ResultActionsForState(false, false, false).next == true);
}

TEST_CASE("A shop purchase after a normal campaign victory continues directly")
{
    CHECK(Campaign_ShouldAdvanceAfterResultShopPurchase(
        /*campaignLevel=*/true, /*playerWon=*/true, /*nextAvailable=*/true,
        /*finaleGreetingPending=*/false
    ));
    CHECK_FALSE(Campaign_ShouldAdvanceAfterResultShopPurchase(true, false, true, false));
    CHECK_FALSE(Campaign_ShouldAdvanceAfterResultShopPurchase(false, true, true, false));
    CHECK_FALSE(Campaign_ShouldAdvanceAfterResultShopPurchase(true, true, false, false));
    CHECK_FALSE(Campaign_ShouldAdvanceAfterResultShopPurchase(true, true, true, true));
}

TEST_CASE("First level 13 win is the special campaign-winning result")
{
    const CampaignResultActions actions =
        Campaign_ResultActionsForState(/*campaignLevel=*/true, /*playerWon=*/true, /*firstCampaignClear=*/true);
    CHECK(actions.repeat == false);
    CHECK(actions.next == true);
    CHECK(Campaign_FinalResultFlowForState(true, false, 13, 13, false) ==
          CampaignFinaleFlow::CelebrationThenResult);
}

TEST_CASE("Later level 13 win is a normal campaign result with repeat and next")
{
    const CampaignResultActions actions =
        Campaign_ResultActionsForState(/*campaignLevel=*/true, /*playerWon=*/true, /*firstCampaignClear=*/false);
    CHECK(actions.repeat == true);
    CHECK(actions.next == true);
    CHECK(Campaign_FinalResultFlowForState(true, false, 13, 13, true) ==
          CampaignFinaleFlow::NormalResult);
}

TEST_CASE("Completed-campaign replays never restart finale fireworks")
{
    CHECK(Campaign_ShouldShowFinaleFireworks(
        /*campaignCompleted=*/true,
        /*postgameFreeplayActive=*/false,
        /*awaitingResultDismissal=*/true
    ));
    CHECK_FALSE(Campaign_ShouldShowFinaleFireworks(
        /*campaignCompleted=*/true,
        /*postgameFreeplayActive=*/false,
        /*awaitingResultDismissal=*/false
    ));
    CHECK_FALSE(Campaign_ShouldShowFinaleFireworks(
        /*campaignCompleted=*/true,
        /*postgameFreeplayActive=*/true,
        /*awaitingResultDismissal=*/true
    ));
}

TEST_CASE("Finale flow matrix celebrates exactly one outcome")
{
    constexpr int levels[] = {1, 3, 12, 13};
    for (const int level : levels)
    {
        for (const bool playerWon : {false, true})
        {
            for (const bool postgameFreeplay : {false, true})
            {
                for (const bool campaignCompleted : {false, true})
                {
                    const bool firstFinalClear = playerWon && !postgameFreeplay &&
                                                 level == 13 && !campaignCompleted;
                    CHECK(Campaign_FinalResultFlowForState(
                        playerWon, postgameFreeplay, level, 13, campaignCompleted
                    ) == (firstFinalClear
                        ? CampaignFinaleFlow::CelebrationThenResult
                        : CampaignFinaleFlow::NormalResult));
                }
            }
        }
    }
}

TEST_CASE("Campaign level selection clears stale finale state before a replay")
{
    // Exact regression: campaign already complete, select Level 3, then win.
    bool awaitingResultDismissal = true;
    Campaign_ClearFinaleStateForLevelSetup(awaitingResultDismissal);
    CHECK_FALSE(awaitingResultDismissal);
    CHECK(Campaign_FinalResultFlowForState(
        /*playerWon=*/true, /*postgameFreeplayActive=*/false,
        /*levelNumber=*/3, /*finalLevelNumber=*/13, /*campaignCompleted=*/true
    ) == CampaignFinaleFlow::NormalResult);
    CHECK_FALSE(Campaign_ShouldShowFinaleFireworks(
        /*campaignCompleted=*/true, /*postgameFreeplayActive=*/false,
        awaitingResultDismissal
    ));
    CHECK(Campaign_ResultDismissFlowForState(awaitingResultDismissal) ==
          CampaignFinaleFlow::NoTransition);
    CHECK(Campaign_ResumeFlowForState(
        /*campaignCompleted=*/true, /*campaignPostgameFreeplayActive=*/false
    ) == CampaignResumeFlow::PostgameFreeplay);
}

TEST_CASE("Finale fireworks require every first-clear handoff condition")
{
    for (const bool campaignCompleted : {false, true})
    {
        for (const bool postgameFreeplay : {false, true})
        {
            for (const bool awaitingResultDismissal : {false, true})
            {
                const bool expected = campaignCompleted && !postgameFreeplay &&
                                      awaitingResultDismissal;
                CHECK(Campaign_ShouldShowFinaleFireworks(
                    campaignCompleted, postgameFreeplay, awaitingResultDismissal
                ) == expected);
            }
        }
    }
}

TEST_CASE("Post-campaign games always use the full tool set")
{
    CHECK(Campaign_PostgameUsesFullToolset(/*postgameFreeplayActive=*/true));
    CHECK_FALSE(Campaign_PostgameUsesFullToolset(/*postgameFreeplayActive=*/false));
    CHECK_FALSE(Campaign_PostgameUsesFullToolset(/*postgameFreeplayActive=*/true, /*enabled=*/false));
    CHECK_FALSE(Campaign_NosEnabledForCampaignLevel(6, /*postgame=*/false));
    CHECK(Campaign_NosEnabledForCampaignLevel(7, /*postgame=*/false));
    CHECK(Campaign_NosEnabledForCampaignLevel(1, /*postgame=*/true));
    CHECK_FALSE(Campaign_NosEnabledForCampaignLevel(1, /*postgame=*/true, /*enabled=*/false));
}

TEST_CASE("Post-campaign tool changes wait for a newly started game")
{
    const CampaignPostgameToolset activeGame = Campaign_SnapshotPostgameToolset(
        /*blocks=*/true, /*nos=*/true
    );
    // Changing the saved preferences does not mutate the snapshot held by the
    // in-progress game.
    const CampaignPostgameToolset nextGame = Campaign_SnapshotPostgameToolset(
        /*blocks=*/false, /*nos=*/false
    );
    CHECK(activeGame.blocksEnabled);
    CHECK(activeGame.nosEnabled);
    CHECK_FALSE(nextGame.blocksEnabled);
    CHECK_FALSE(nextGame.nosEnabled);
}

TEST_CASE("Post-campaign result button always says next")
{
    CHECK(Campaign_ResultButtonUsesNext(/*postCampaign=*/true, /*won=*/true));
    CHECK(Campaign_ResultButtonUsesNext(/*postCampaign=*/true, /*won=*/false));
    CHECK(Campaign_ResultButtonUsesNext(/*postCampaign=*/false, /*won=*/true));
    CHECK_FALSE(Campaign_ResultButtonUsesNext(/*postCampaign=*/false, /*won=*/false));
}

TEST_CASE("Level 13 loss before campaign completion offers repeat only")
{
    const CampaignResultActions actions =
        Campaign_ResultActionsForState(/*campaignLevel=*/true, /*playerWon=*/false, /*firstCampaignClear=*/false);
    CHECK(actions.repeat == true);
    CHECK(actions.next == false);
    CHECK(Campaign_FinalResultFlowForState(false, false, 13, 13, false) ==
          CampaignFinaleFlow::NormalResult);
}

TEST_CASE("Post-campaign level 13 outcomes use random-level next-only buttons")
{
    const CampaignResultActions win =
        Campaign_ResultActionsForState(/*campaignLevel=*/false, /*playerWon=*/true, /*firstCampaignClear=*/false);
    const CampaignResultActions loss =
        Campaign_ResultActionsForState(/*campaignLevel=*/false, /*playerWon=*/false, /*firstCampaignClear=*/false);
    CHECK(win.repeat == false);
    CHECK(win.next == true);
    CHECK(loss.repeat == false);
    CHECK(loss.next == true);
    CHECK(Campaign_FinalResultFlowForState(true, true, 13, 13, true) ==
          CampaignFinaleFlow::NormalResult);
}
