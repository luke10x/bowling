#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../storyline.h"

TEST_CASE("Story: first solo outro node chosen by score threshold")
{
    CHECK(Story_FirstSoloOutroIdForScore(0) == 10);
    CHECK(Story_FirstSoloOutroIdForScore(99) == 10);
    CHECK(Story_FirstSoloOutroIdForScore(100) == 20);
    CHECK(Story_FirstSoloOutroIdForScore(150) == 20);
}

TEST_CASE("Story: lose path forces school via EVENT_GO_TO_SCHOOL")
{
    const StorylineNode *n = Story_FindNode(11);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_FIRST_FAIL_GO_SCHOOL);

    const StoryChoiceOption *opt = Story_FindFirstOptionByChoiceId(CHOICE_FIRST_FAIL_GO_SCHOOL);
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
