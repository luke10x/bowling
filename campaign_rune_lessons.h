#pragma once

// Persisted campaign flags for the one-time Angel explanations attached to
// chest rune rewards.
enum CampaignRuneLessonFlag
{
    CAMPAIGN_RUNE_LESSON_FIRST_RUNE = 1 << 0,
    CAMPAIGN_RUNE_LESSON_BOOM = 1 << 1,
    CAMPAIGN_RUNE_LESSON_BOLT = 1 << 2,
};

enum class CampaignRuneLessonPickup
{
    Other,
    Boom,
    Bolt,
};

struct CampaignRuneLessonDecision
{
    int storyId = 0;
    int updatedFlags = 0;
};

// Story IDs 3053 and 3054 are chained two-message variants: first-rune
// guidance followed by the Boom or Bolt warning respectively.
static inline CampaignRuneLessonDecision CampaignRuneLesson_OnPickup(
    int seenFlags,
    CampaignRuneLessonPickup pickup)
{
    CampaignRuneLessonDecision decision = {0, seenFlags};
    const bool firstRuneSeen =
        (seenFlags & CAMPAIGN_RUNE_LESSON_FIRST_RUNE) != 0;
    const bool boomSeen =
        (seenFlags & CAMPAIGN_RUNE_LESSON_BOOM) != 0;
    const bool boltSeen =
        (seenFlags & CAMPAIGN_RUNE_LESSON_BOLT) != 0;

    if (!firstRuneSeen)
    {
        decision.updatedFlags |= CAMPAIGN_RUNE_LESSON_FIRST_RUNE;
        if (pickup == CampaignRuneLessonPickup::Boom && !boomSeen)
        {
            decision.updatedFlags |= CAMPAIGN_RUNE_LESSON_BOOM;
            decision.storyId = 3053;
        }
        else if (pickup == CampaignRuneLessonPickup::Bolt && !boltSeen)
        {
            decision.updatedFlags |= CAMPAIGN_RUNE_LESSON_BOLT;
            decision.storyId = 3054;
        }
        else
        {
            decision.storyId = 3050;
        }
        return decision;
    }

    if (pickup == CampaignRuneLessonPickup::Boom && !boomSeen)
    {
        decision.updatedFlags |= CAMPAIGN_RUNE_LESSON_BOOM;
        decision.storyId = 3051;
    }
    else if (pickup == CampaignRuneLessonPickup::Bolt && !boltSeen)
    {
        decision.updatedFlags |= CAMPAIGN_RUNE_LESSON_BOLT;
        decision.storyId = 3052;
    }
    return decision;
}
