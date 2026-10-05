#pragma once

enum class CampaignResumeFlow
{
    CurrentLevel = 0,
    CompletedSummary = 1,
    PostgameFreeplay = 2,
};

// The first clear of the final chapter has a deliberately distinct sequence:
// celebration, ordinary match results, then the Angel greeting.  Keeping these
// choices here makes the state-machine contract testable without the renderer.
enum class CampaignFinaleFlow
{
    NormalResult = 0,
    CelebrationThenResult = 1,
    AngelGreeting = 2,
    NoTransition = 3,
};

enum class CampaignVisualBiomeSource
{
    CurrentCampaignLevel = 0,
    BonusSource = 1,
};

enum class CampaignResetScope
{
    CampaignOnly = 0,
    Factory = 1,
};

struct CampaignResultActions
{
    bool repeat = false;
    bool next = false;
};

static inline CampaignResultActions Campaign_ResultActionsForState(
    bool campaignLevel,
    bool playerWon,
    bool firstCampaignClear)
{
    if (!campaignLevel)
        return {false, true};
    if (firstCampaignClear)
        return {false, true};
    if (playerWon)
        return {true, true};
    return {true, false};
}

// Buying from the Shop opened by a normal campaign victory is a detour on the
// way to the next level, not a second result decision.  The first Level 13
// clear is excluded because its result must lead into the finale greeting.
static inline bool Campaign_ShouldAdvanceAfterResultShopPurchase(
    bool campaignLevel,
    bool playerWon,
    bool nextAvailable,
    bool finaleGreetingPending)
{
    return campaignLevel && playerWon && nextAvailable && !finaleGreetingPending;
}

static inline bool CampaignReset_PreservesBallInventory(CampaignResetScope scope)
{
    return scope == CampaignResetScope::CampaignOnly;
}

// Campaign progress advances as soon as a level is won, but its optional bonus
// round must retain the completed level's environment while its choice modal is
// visible and while the round is active.
static inline CampaignVisualBiomeSource Campaign_VisualBiomeSourceForState(
    bool bonusQueued,
    bool bonusActive)
{
    return bonusQueued || bonusActive
        ? CampaignVisualBiomeSource::BonusSource
        : CampaignVisualBiomeSource::CurrentCampaignLevel;
}

static inline bool Campaign_ShouldOfferLevelBonus(
    int levelNumber,
    int finalLevelNumber,
    bool bonusConfigured,
    bool alreadyGranted)
{
    return levelNumber > 0 &&
           levelNumber < finalLevelNumber &&
           bonusConfigured &&
           !alreadyGranted;
}

static inline CampaignFinaleFlow Campaign_FinalResultFlowForState(
    bool playerWon,
    bool postgameFreeplayActive,
    int levelNumber,
    int finalLevelNumber,
    bool campaignCompleted)
{
    return playerWon &&
           !postgameFreeplayActive &&
           levelNumber == finalLevelNumber &&
           !campaignCompleted
        ? CampaignFinaleFlow::CelebrationThenResult
        : CampaignFinaleFlow::NormalResult;
}

static inline CampaignFinaleFlow Campaign_FinaleCloseFlowForState(bool awaitingResultDismissal)
{
    return awaitingResultDismissal
        ? CampaignFinaleFlow::NormalResult
        : CampaignFinaleFlow::AngelGreeting;
}

static inline CampaignFinaleFlow Campaign_ResultDismissFlowForState(bool awaitingResultDismissal)
{
    return awaitingResultDismissal
        ? CampaignFinaleFlow::AngelGreeting
        : CampaignFinaleFlow::NoTransition;
}

// Fireworks belong only to the first-clear sequence: the campaign-complete
// modal and the result screen immediately following it. Completed-campaign
// level replays are ordinary results and must not restart the celebration.
static inline bool Campaign_ShouldShowFinaleFireworks(
    bool campaignCompleted,
    bool postgameFreeplayActive,
    bool awaitingResultDismissal)
{
    return campaignCompleted && !postgameFreeplayActive && awaitingResultDismissal;
}

// Random post-campaign games are intentionally fully equipped, regardless of
// the last campaign chapter the player replayed before returning here.
static inline bool Campaign_PostgameUsesFullToolset(
    bool postgameFreeplayActive,
    bool postgameToolEnabled = true)
{
    return postgameFreeplayActive && postgameToolEnabled;
}

struct CampaignPostgameToolset
{
    bool blocksEnabled = true;
    bool nosEnabled = true;
};

// Settings are preferences for the next post-campaign game.  A game takes this
// value only at setup, so opening and closing Campaign cannot reconfigure a
// live match.
static inline CampaignPostgameToolset Campaign_SnapshotPostgameToolset(
    bool configuredBlocksEnabled,
    bool configuredNosEnabled)
{
    return {configuredBlocksEnabled, configuredNosEnabled};
}

// Completed-campaign results always advance to a fresh random post-campaign
// game.  Their button must say NEXT even if the just-finished game was lost.
static inline bool Campaign_ResultButtonUsesNext(bool postCampaignResult, bool playerWon)
{
    return postCampaignResult || playerWon;
}

static inline bool Campaign_NosEnabledForCampaignLevel(
    int levelNumber,
    bool postgameFreeplayActive,
    bool postgameNosEnabled = true)
{
    return Campaign_PostgameUsesFullToolset(postgameFreeplayActive, postgameNosEnabled) || levelNumber >= 7;
}

// Selecting a campaign level starts a fresh match. It must discard the
// one-shot first-clear handoff state left by an earlier campaign completion.
static inline void Campaign_ClearFinaleStateForLevelSetup(bool &awaitingResultDismissal)
{
    awaitingResultDismissal = false;
}

static inline CampaignResumeFlow Campaign_ResumeFlowForState(bool campaignCompleted, bool campaignPostgameFreeplayActive)
{
    // The completion layout is queued explicitly for the one first-clear
    // handoff. A saved completed campaign, including a completed-level replay
    // after its result, resumes the post-campaign random flow instead.
    if (campaignCompleted || campaignPostgameFreeplayActive)
        return CampaignResumeFlow::PostgameFreeplay;
    return CampaignResumeFlow::CurrentLevel;
}

static inline int Campaign_StartStoryIdForState(
    int levelNumber,
    int configuredStartStoryId,
    int attemptCountBeforeThisSetup,
    bool schoolDone,
    bool campaignCompleted,
    bool campaignPostgameFreeplayActive)
{
    if (Campaign_ResumeFlowForState(campaignCompleted, campaignPostgameFreeplayActive) !=
        CampaignResumeFlow::CurrentLevel)
        return 0;

    if (levelNumber == 1 && attemptCountBeforeThisSetup <= 0)
        return 0;

    if (levelNumber == 1 && schoolDone && configuredStartStoryId == 40)
        return 41;

    if (levelNumber == 2 && !schoolDone)
        return 30020;

    return configuredStartStoryId;
}
