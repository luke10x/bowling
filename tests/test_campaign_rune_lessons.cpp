#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_rune_lessons.h"

TEST_CASE("First non-special rune gives only the general casting lesson")
{
    const auto lesson = CampaignRuneLesson_OnPickup(0, CampaignRuneLessonPickup::Other);
    CHECK(lesson.storyId == 3050);
    CHECK(lesson.updatedFlags == CAMPAIGN_RUNE_LESSON_FIRST_RUNE);
}

TEST_CASE("First Boom or Bolt combines the general and special lessons")
{
    const auto boom = CampaignRuneLesson_OnPickup(0, CampaignRuneLessonPickup::Boom);
    CHECK(boom.storyId == 3053);
    CHECK(boom.updatedFlags ==
          (CAMPAIGN_RUNE_LESSON_FIRST_RUNE | CAMPAIGN_RUNE_LESSON_BOOM));

    const auto bolt = CampaignRuneLesson_OnPickup(0, CampaignRuneLessonPickup::Bolt);
    CHECK(bolt.storyId == 3054);
    CHECK(bolt.updatedFlags ==
          (CAMPAIGN_RUNE_LESSON_FIRST_RUNE | CAMPAIGN_RUNE_LESSON_BOLT));
}

TEST_CASE("Later Boom and Bolt lessons appear once independently")
{
    int flags = CAMPAIGN_RUNE_LESSON_FIRST_RUNE;
    const auto boom = CampaignRuneLesson_OnPickup(flags, CampaignRuneLessonPickup::Boom);
    CHECK(boom.storyId == 3051);
    flags = boom.updatedFlags;

    const auto repeatBoom = CampaignRuneLesson_OnPickup(flags, CampaignRuneLessonPickup::Boom);
    CHECK(repeatBoom.storyId == 0);
    CHECK(repeatBoom.updatedFlags == flags);

    const auto bolt = CampaignRuneLesson_OnPickup(flags, CampaignRuneLessonPickup::Bolt);
    CHECK(bolt.storyId == 3052);
    CHECK(bolt.updatedFlags ==
          (CAMPAIGN_RUNE_LESSON_FIRST_RUNE | CAMPAIGN_RUNE_LESSON_BOOM |
           CAMPAIGN_RUNE_LESSON_BOLT));
}
