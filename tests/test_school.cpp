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
