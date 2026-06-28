#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_endgame_buf.h"

TEST_CASE("Endgame mana buff ramps across the last five campaign levels")
{
    CHECK(Campaign_EndgameBufForLevel(1) == doctest::Approx(1.00f));
    CHECK(Campaign_EndgameBufForLevel(8) == doctest::Approx(1.00f));
    CHECK(Campaign_EndgameBufForLevel(9) == doctest::Approx(1.10f));
    CHECK(Campaign_EndgameBufForLevel(10) == doctest::Approx(1.25f));
    CHECK(Campaign_EndgameBufForLevel(11) == doctest::Approx(1.50f));
    CHECK(Campaign_EndgameBufForLevel(12) == doctest::Approx(1.75f));
    CHECK(Campaign_EndgameBufForLevel(13) == doctest::Approx(2.00f));
    CHECK(Campaign_EndgameBufForLevel(99) == doctest::Approx(1.00f));
}

