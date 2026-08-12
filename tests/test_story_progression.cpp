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

TEST_CASE("Campaign start stories skip only the truly fresh level 1 boot")
{
    CHECK(Campaign_StartStoryIdForState(1, 40, 0, false, false, false) == 0);
    CHECK(Campaign_StartStoryIdForState(1, 40, 1, false, false, false) == 40);
    CHECK(Campaign_StartStoryIdForState(1, 40, 1, true, false, false) == 40);
}

TEST_CASE("Campaign start stories respect school and completed resume flow")
{
    CHECK(Campaign_StartStoryIdForState(2, 3002, 0, false, false, false) == 30020);
    CHECK(Campaign_StartStoryIdForState(2, 3002, 0, true, false, false) == 3002);
    CHECK(Campaign_StartStoryIdForState(13, 3012, 3, true, true, false) == 0);
    CHECK(Campaign_StartStoryIdForState(13, 3012, 3, true, true, true) == 0);
}

TEST_CASE("Campaign routed start story nodes exist")
{
    REQUIRE(Story_FindNode(40) != nullptr);
    REQUIRE(Story_FindNode(30020) != nullptr);
}
