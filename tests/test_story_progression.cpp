#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_completion_flow.h"
#include "../storyline.h"

TEST_CASE("Story: first solo outro node chosen by score threshold")
{
    CHECK(Story_FirstSoloOutroIdForScore(0) == 10);
    CHECK(Story_FirstSoloOutroIdForScore(99) == 10);
    CHECK(Story_FirstSoloOutroIdForScore(100) == 20);
    CHECK(Story_FirstSoloOutroIdForScore(150) == 20);
}

TEST_CASE("Story: lose path offers optional school via EVENT_GO_TO_SCHOOL")
{
    const StorylineNode *n = Story_FindNode(11);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_LEVEL1_SCHOOL_OFFER);

    const StoryChoiceOption *opt = Story_FindFirstOptionByChoiceId(CHOICE_LEVEL1_SCHOOL_OFFER);
    REQUIRE(opt != nullptr);
    CHECK(opt->trigger_event == EVENT_GO_TO_SCHOOL);
}

TEST_CASE("Story: win path can route to BOT via EVENT_GO_TO_BOT")
{
    const StorylineNode *n = Story_FindNode(21);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_FIRST_WIN_NEXT);

    bool found = false;
    for (int32_t i = 0; i < STORY_OPTIONS_COUNT; i++)
    {
        const StoryChoiceOption &opt = STORY_OPTIONS[i];
        if (opt.choice_id == CHOICE_FIRST_WIN_NEXT && opt.trigger_event == EVENT_GO_TO_BOT)
            found = true;
    }
    CHECK(found);
}

TEST_CASE("Campaign school reminder only appears on a failed level 1 replay")
{
    CHECK(Campaign_StartStoryIdForState(1, 40, 0, false, false, false, false) == 0);
    CHECK(Campaign_StartStoryIdForState(1, 40, 1, false, false, false, false) == 40);
    CHECK(Campaign_StartStoryIdForState(1, 40, 1, true, false, false, false) == 0);
    CHECK(Campaign_StartStoryIdForState(1, 40, 1, false, true, false, false) == 0);
}

TEST_CASE("Persisted campaign resume preserves both valid Level 1 completion routes")
{
    // A graduate with an old/reset level-1 save gets the synthetic pass.
    CHECK(Campaign_ShouldApplySchoolPassOnResume(true, 1, 0));
    // A normal 100-point pass already stored level 2 and must remain there.
    CHECK_FALSE(Campaign_ShouldApplySchoolPassOnResume(false, 2, 1));
    // A previously stored school pass also remains at level 2 on later loads.
    CHECK_FALSE(Campaign_ShouldApplySchoolPassOnResume(true, 2, 1));
    // Graduation never rewrites a genuine Level 1 score/win.
    CHECK_FALSE(Campaign_ShouldApplySchoolPassOnResume(true, 1, 1));
}

TEST_CASE("Campaign start stories respect school and completed resume flow")
{
    CHECK(Campaign_StartStoryIdForState(2, 3002, 0, false, true, false, false) == 3002);
    CHECK(Campaign_StartStoryIdForState(2, 3002, 0, true, false, false, false) == 3002);
    CHECK(Campaign_StartStoryIdForState(13, 3012, 3, true, false, true, false) == 0);
    CHECK(Campaign_StartStoryIdForState(13, 3012, 3, true, false, true, true) == 0);
}

TEST_CASE("Campaign routed start story nodes exist")
{
    REQUIRE(Story_FindNode(40) != nullptr);
    REQUIRE(Story_FindNode(41) != nullptr);
    REQUIRE(Story_FindNode(30020) != nullptr);
}

TEST_CASE("Rune lesson story chains keep first-rune guidance before warnings")
{
    const StorylineNode *firstBoom = Story_FindNode(3053);
    const StorylineNode *firstBolt = Story_FindNode(3054);
    REQUIRE(firstBoom != nullptr);
    REQUIRE(firstBolt != nullptr);
    CHECK(firstBoom->next_storyline == 3051);
    CHECK(firstBolt->next_storyline == 3052);
    CHECK(Story_FindNode(3050) != nullptr);
    CHECK(Story_FindNode(3051) != nullptr);
    CHECK(Story_FindNode(3052) != nullptr);
}

TEST_CASE("Completed-school level 1 intro does not offer school again")
{
    const StorylineNode *n = Story_FindNode(41);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_SCHOOL_OK);
    CHECK(n->next_storyline == 0);
}

TEST_CASE("Completed-school first reveal has no school offer")
{
    const StorylineNode *n = Story_FindNode(22);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_SCHOOL_OK);
    CHECK(n->next_storyline == 0);
}

TEST_CASE("School manual lesson switching requires confirmation")
{
    const StorylineNode *switchNode = Story_FindNode(31);
    REQUIRE(switchNode != nullptr);
    CHECK(switchNode->choice_group == CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM);

    const StorylineNode *restartNode = Story_FindNode(32);
    REQUIRE(restartNode != nullptr);
    CHECK(restartNode->choice_group == CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM);

    bool foundConfirm = false;
    bool foundCancel = false;
    for (int32_t i = 0; i < STORY_OPTIONS_COUNT; i++)
    {
        const StoryChoiceOption &opt = STORY_OPTIONS[i];
        if (opt.choice_id != CHOICE_SCHOOL_LESSON_SWITCH_CONFIRM)
            continue;
        if (opt.trigger_event == EVENT_SCHOOL_CONFIRM_LESSON_SWITCH)
            foundConfirm = true;
        if (opt.trigger_event == EVENT_SCHOOL_CANCEL_LESSON_SWITCH)
            foundCancel = true;
    }
    CHECK(foundConfirm);
    CHECK(foundCancel);
}
