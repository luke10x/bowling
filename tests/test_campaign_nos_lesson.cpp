#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_nos_lesson.h"

TEST_CASE("NOS lesson prompts only after three Level 7 player frames")
{
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(6, 9, false, false));
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(7, 2, false, false));
    CHECK(CampaignNosLesson_ShouldPrompt(7, 3, false, false));
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(7, 3, true, false));
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(7, 3, false, true));
}

TEST_CASE("NOS lesson white blink stops only when the player learns NOS")
{
    CHECK(CampaignNosLesson_ShouldWhiteBlink(
        /*learned=*/false, /*lessonBlinkActive=*/true,
        /*nosHeld=*/false, /*charge01=*/1.0f));
    CHECK_FALSE(CampaignNosLesson_ShouldWhiteBlink(false, true, true, 1.0f));
    CHECK_FALSE(CampaignNosLesson_ShouldWhiteBlink(false, true, false, 0.0f));
    CHECK_FALSE(CampaignNosLesson_ShouldWhiteBlink(true, true, false, 1.0f));
}

TEST_CASE("NOS lesson refills only a meter below half charge")
{
    CHECK(CampaignNosLesson_ShouldRefill(0.0f));
    CHECK(CampaignNosLesson_ShouldRefill(0.49f));
    CHECK_FALSE(CampaignNosLesson_ShouldRefill(0.5f));
    CHECK_FALSE(CampaignNosLesson_ShouldRefill(1.0f));
}
