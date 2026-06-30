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
