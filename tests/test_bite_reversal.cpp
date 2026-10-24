#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../bite_reversal.h"

TEST_CASE("Low bite needs twice the base counter-spin input")
{
    CHECK(BiteReversal::SideDrive(0.1f, -0.03f, 0.0f) == doctest::Approx(0.05f));
    CHECK(BiteReversal::SideDrive(0.1f, -0.03f, 1.0f) >= 0.1f);
    CHECK(BiteReversal::SideDrive(-0.1f, 0.03f, 0.0f) == doctest::Approx(-0.05f));
}

TEST_CASE("High bite can reverse more strongly with slower counter-spin")
{
    // Values near the low and high ends of the actual ball catalog.
    const float lowBiteStrongInput = BiteReversal::SideDrive(0.1f, -0.2f, 0.34f);
    const float highBiteWeakInput = BiteReversal::SideDrive(0.05f, -0.2f, 0.97f);
    CHECK(highBiteWeakInput > lowBiteStrongInput);
    CHECK(highBiteWeakInput <= 0.15f + 1e-6f);
}

TEST_CASE("Bite reversal does not alter aligned drift or amplify a tiny input without bound")
{
    CHECK(BiteReversal::SideDrive(0.1f, 0.2f, 1.0f) == doctest::Approx(0.1f));
    CHECK(BiteReversal::SideDrive(0.0f, -0.2f, 1.0f) == doctest::Approx(0.0f));
    CHECK(BiteReversal::SideDrive(0.001f, -0.2f, 1.0f) <= 0.003f + 1e-6f);
}
