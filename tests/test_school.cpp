#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../coins.h"
#include "../school/school_rules.h"

TEST_CASE("School_LessonHasPins: lesson 3 has no pins")
{
    CHECK(School_LessonHasPins(1) == true);
    CHECK(School_LessonHasPins(2) == true);
    CHECK(School_LessonHasPins(3) == false);
    CHECK(School_LessonHasPins(4) == true);
    CHECK(School_LessonHasPins(5) == true);
}

TEST_CASE("School strike lesson keeps gem visuals isolated")
{
    CoinLane lane {};
    lane.visualKind = CollectableVisualKind::Gem;
    lane.initStars(CoinPattern::Static, 3);
    CHECK(lane.visualKind == CollectableVisualKind::Gem);
}

TEST_CASE("School strike swap delay grows along lane")
{
    CHECK(School_StrikeSwapDelayForZ(-16.0f, -16.0f, -0.2f, 1.0f) == doctest::Approx(0.0f));
    CHECK(School_StrikeSwapDelayForZ(-0.2f, -16.0f, -0.2f, 1.0f) == doctest::Approx(1.0f));
    CHECK(School_StrikeSwapDelayForZ(-8.1f, -16.0f, -0.2f, 1.0f) == doctest::Approx(0.5f).epsilon(0.05f));
}
