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

static inline CampaignResumeFlow Campaign_ResumeFlowForState(bool campaignCompleted, bool campaignPostgameFreeplayActive)
{
    if (campaignPostgameFreeplayActive)
        return CampaignResumeFlow::PostgameFreeplay;
    if (campaignCompleted)
        return CampaignResumeFlow::CompletedSummary;
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
