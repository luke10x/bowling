#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_nos_lesson.h"

TEST_CASE("NOS reminder prompts once per Level 7 run after three player frames")
{
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(6, 9, false));
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(7, 2, false));
    CHECK(CampaignNosLesson_ShouldPrompt(7, 3, false));
    CHECK_FALSE(CampaignNosLesson_ShouldPrompt(7, 3, true));
}

TEST_CASE("NOS reminder white blink stops when NOS is pressed")
{
    CHECK(CampaignNosLesson_ShouldWhiteBlink(
        /*lessonBlinkActive=*/true, /*nosHeld=*/false, /*charge01=*/1.0f));
    CHECK_FALSE(CampaignNosLesson_ShouldWhiteBlink(true, true, 1.0f));
    CHECK_FALSE(CampaignNosLesson_ShouldWhiteBlink(true, false, 0.0f));
}

TEST_CASE("NOS lesson refills only a meter below half charge")
{
    CHECK(CampaignNosLesson_ShouldRefill(0.0f));
    CHECK(CampaignNosLesson_ShouldRefill(0.49f));
    CHECK_FALSE(CampaignNosLesson_ShouldRefill(0.5f));
    CHECK_FALSE(CampaignNosLesson_ShouldRefill(1.0f));
}
