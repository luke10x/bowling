#pragma once

enum class CampaignResumeFlow
{
    CurrentLevel = 0,
    CompletedSummary = 1,
    PostgameFreeplay = 2,
};

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

    if (levelNumber == 2 && !schoolDone)
        return 30020;

    return configuredStartStoryId;
}
