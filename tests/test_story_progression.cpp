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
    REQUIRE(Story_FindNode(30020) != nullptr);
}

TEST_CASE("Post-campaign dialog only continues into free play")
{
    const StorylineNode *n = Story_FindNode(32000);
    REQUIRE(n != nullptr);
    CHECK(n->choice_group == CHOICE_CAMPAIGN_ENDGAME);
    CHECK(EVENT_CAMPAIGN_POSTGAME_CONTINUE != EVENT_CONTINUE_CAMPAIGN_AFTER_SHOP_OFFER);

    int optionCount = 0;
    for (int32_t i = 0; i < STORY_OPTIONS_COUNT; ++i)
    {
        const StoryChoiceOption &opt = STORY_OPTIONS[i];
        if (opt.choice_id != CHOICE_CAMPAIGN_ENDGAME)
            continue;
        ++optionCount;
        CHECK(opt.trigger_event == EVENT_CAMPAIGN_POSTGAME_CONTINUE);
    }
    CHECK(optionCount == 1);
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

TEST_CASE("Story dialogue nodes use their corresponding complete TXL entries")
{
    const TxlKey expectedKeys[] = {
        TXL_STORY_1,
        TXL_STORY_2,
        TXL_STORY_10,
        TXL_STORY_11,
        TXL_STORY_20,
        TXL_STORY_21,
        TXL_STORY_22,
        TXL_STORY_30,
        TXL_STORY_31,
        TXL_STORY_32,
        TXL_STORY_1000,
        TXL_STORY_1010,
        TXL_STORY_1012,
        TXL_STORY_1013,
        TXL_STORY_1014,
        TXL_STORY_1030,
        TXL_STORY_1020,
        TXL_STORY_1022,
        TXL_STORY_1052,
        TXL_STORY_1054,
        TXL_STORY_1060,
        TXL_STORY_1032,
        TXL_STORY_1040,
        TXL_STORY_1070,
        TXL_STORY_1072,
        TXL_STORY_1080,
        TXL_STORY_1021,
        TXL_STORY_40,
        TXL_STORY_30020,
        TXL_STORY_3002,
        TXL_STORY_3102,
        TXL_STORY_3003,
        TXL_STORY_30031,
        TXL_STORY_30032,
        TXL_STORY_3042,
        TXL_STORY_3050,
        TXL_STORY_3051,
        TXL_STORY_3052,
        TXL_STORY_3053,
        TXL_STORY_3054,
        TXL_STORY_3103,
        TXL_STORY_3004,
        TXL_STORY_3104,
        TXL_STORY_3040,
        TXL_STORY_3041,
        TXL_STORY_3140,
        TXL_STORY_3141,
        TXL_STORY_3005,
        TXL_STORY_3105,
        TXL_STORY_3006,
        TXL_STORY_30061,
        TXL_STORY_3106,
        TXL_STORY_3007,
        TXL_STORY_3107,
        TXL_STORY_3008,
        TXL_STORY_3108,
        TXL_STORY_3009,
        TXL_STORY_3109,
        TXL_STORY_3010,
        TXL_STORY_3110,
        TXL_STORY_3011,
        TXL_STORY_3111,
        TXL_STORY_3012,
        TXL_STORY_3112,
        TXL_STORY_32000,
    };
    REQUIRE(sizeof(expectedKeys) / sizeof(expectedKeys[0]) == STORYLINES_COUNT);
    const TxlLanguage languages[] = {
        TXL_LANG_EN_US, TXL_LANG_LT_LT, TXL_LANG_JP_JP, TXL_LANG_ZH_CN,
    };
    for (int32_t i = 0; i < STORYLINES_COUNT; ++i)
    {
        const auto &node = STORYLINES[i];
        CAPTURE(node.storyline_id);
        const TxlKey key = Story_TextKey(node.storyline_id);
        REQUIRE(key >= 0);
        REQUIRE(key < TXL_KEY_COUNT);
        CHECK(key == expectedKeys[i]);
        CHECK(node.text[0] == '\0');
        for (const auto language : languages)
        {
            CAPTURE(language);
            const char *translated = Txl_Get(language, key);
            REQUIRE(translated != nullptr);
            CHECK(translated[0] != '\0');
            CHECK(Story_Text(language, node.storyline_id, "fallback") == translated);
        }
    }
}

TEST_CASE("Unknown story IDs preserve the caller's fallback")
{
    const char *fallback = "fallback";
    CHECK(Story_TextKey(-1) == TXL_KEY_COUNT);
    for (int language = 0; language < TXL_LANG_COUNT; ++language)
    {
        CHECK(Story_Text((TxlLanguage)language, -1, fallback) == fallback);
        CHECK(Story_Text((TxlLanguage)language, -1, nullptr) == nullptr);
    }
}

TEST_CASE("Story choice and speaker keys are populated in every locale")
{
    for (int language = 0; language < TXL_LANG_COUNT; ++language)
    {
        const auto lang = (TxlLanguage)language;
        for (const auto &option : STORY_OPTIONS)
        {
            REQUIRE(option.option >= 0);
            REQUIRE(option.option < TXL_KEY_COUNT);
            CHECK(Story_OptionText(lang, option)[0] != '\0');
        }
        for (int speaker = SPEAKER_DEVIL; speaker <= SPEAKER_ANGEL; ++speaker)
            CHECK(Story_SpeakerName(lang, speaker)[0] != '\0');
    }
}
