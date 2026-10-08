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

// Tool reminders teach a mechanic on its introductory campaign level. They
// are per-run, not saved knowledge: replaying that level repeats the prompt.
enum class CampaignToolReminder
{
    Glass,
    Nos,
};

static inline bool Campaign_ShouldPromptToolReminder(
    CampaignToolReminder reminder,
    int levelNumber,
    int completedPlayerFrame,
    bool promptedThisRun)
{
    constexpr int kPromptFrame = 3;
    const int teachingLevel = reminder == CampaignToolReminder::Glass ? 5 : 7;
    return levelNumber == teachingLevel &&
           completedPlayerFrame >= kPromptFrame &&
           !promptedThisRun;
}

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

// The Shop opened from a normal campaign result is a detour, not another
// result decision. Closing it or buying a ball resumes the campaign: a win
// starts the next level and a loss repeats the current one. The first Level
// 13 clear remains excluded because its result must lead into the finale.
static inline bool Campaign_ShouldResumeAfterResultShopExit(
    bool campaignLevel,
    bool finaleGreetingPending)
{
    return campaignLevel && !finaleGreetingPending;
}

// The optional shop offered by a campaign end-story is a detour between two
// chapters, never a second result decision. Whether the player buys, closes
// the shop, or chooses "Later", continue along the winning path.
static inline bool Campaign_ShouldAdvanceAfterEndStoryShopDecision(
    bool campaignLevel,
    bool playerWon,
    bool finaleGreetingPending)
{
    return campaignLevel && playerWon && !finaleGreetingPending;
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
    bool alreadyGranted,
    int winsAfterThisResult = 1)
{
    return levelNumber > 0 &&
        levelNumber < finalLevelNumber &&
        bonusConfigured &&
        !alreadyGranted &&
        winsAfterThisResult <= 1;
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

// A school graduate only needs the synthetic Level 1 pass when an older or
// freshly-reset campaign save still points at level 1.  A normal Level 1 win
// already saved level 2, so leave that persisted progress untouched.
static inline bool Campaign_ShouldApplySchoolPassOnResume(
    bool schoolDone,
    int savedCampaignLevel,
    int savedLevelOneWins)
{
    return schoolDone && savedCampaignLevel == 1 && savedLevelOneWins <= 0;
}

static inline int Campaign_StartStoryIdForState(
    int levelNumber,
    int configuredStartStoryId,
    int attemptCountBeforeThisSetup,
    bool schoolDone,
    bool levelOnePassed,
    bool campaignCompleted,
    bool campaignPostgameFreeplayActive)
{
    if (Campaign_ResumeFlowForState(campaignCompleted, campaignPostgameFreeplayActive) !=
        CampaignResumeFlow::CurrentLevel)
        return 0;

    // School is a recovery lesson, not an opening interruption.  The Angel
    // only mentions it after a player has failed the first milestone and
    // deliberately starts level 1 again.  A passed milestone (including the
    // school-graduation pass) and a finished school silence that reminder.
    if (levelNumber == 1)
    {
        if (schoolDone || levelOnePassed || attemptCountBeforeThisSetup <= 0)
            return 0;
        return configuredStartStoryId;
    }

    // Reaching level 2 proves the first milestone was cleared, so never
    // reintroduce the school there.
    if (levelNumber == 2 && !levelOnePassed && !schoolDone)
        return 30020;

    return configuredStartStoryId;
}
