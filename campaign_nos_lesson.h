#pragma once

#include "campaign_completion_flow.h"

// Level 6's outro announces NOS; Level 7 is the first campaign game where
// the player can actually use it. Keep this small policy separate from UI so
// its progression rules remain easy to test.
static constexpr int kCampaignNosLessonLevel = 7;
static constexpr int kCampaignNosLessonStoryId = 30061;
static inline bool CampaignNosLesson_ShouldPrompt(
    int levelNumber,
    int completedPlayerFrame,
    bool promptedThisLevel)
{
    return Campaign_ShouldPromptToolReminder(
        CampaignToolReminder::Nos, levelNumber, completedPlayerFrame, promptedThisLevel
    );
}

static inline bool CampaignNosLesson_ShouldWhiteBlink(
    bool lessonBlinkActive,
    bool nosHeld,
    float charge01)
{
    return lessonBlinkActive && !nosHeld && charge01 > 0.001f;
}

static inline bool CampaignNosLesson_ShouldRefill(float charge01)
{
    return charge01 < 0.5f;
}
