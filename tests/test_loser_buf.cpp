#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../loser_buf.h"

TEST_CASE("ordinary and strike lesson thresholds")
{
    CHECK(LoserBuf_ValueForMisses(4, false) == doctest::Approx(0.0f));
    CHECK(LoserBuf_ValueForMisses(5, false) == doctest::Approx(0.1f));
    CHECK(LoserBuf_ValueForMisses(2, true) == doctest::Approx(0.0f));
    CHECK(LoserBuf_ValueForMisses(3, true) == doctest::Approx(0.1f));
}

TEST_CASE("configured progression reaches one without overshooting")
{
    const float expected[] = {
        .10f, .10f, .20f, .30f, .40f, .45f, .50f, .55f, .60f,
        .65f, .67f, .70f, .72f, .74f, .76f, .78f, .80f
    };
    for (int i = 0; i < 17; ++i)
        CHECK(LoserBuf_ValueForMisses(5 + i, false) == doctest::Approx(expected[i]));
    CHECK(LoserBuf_ValueForMisses(22, false) == doctest::Approx(.81f));
    CHECK(LoserBuf_ValueForMisses(31, false) == doctest::Approx(.90f));
    CHECK(LoserBuf_ValueForMisses(32, false) == doctest::Approx(.95f));
    CHECK(LoserBuf_ValueForMisses(33, false) == doctest::Approx(1.0f));
    CHECK(LoserBuf_ValueForMisses(1000, false) == doctest::Approx(1.0f));
}

TEST_CASE("buff multipliers return to baseline at zero")
{
    CHECK(LoserBuf_HitMultiplier(0.0f) == doctest::Approx(1.0f));
    CHECK(LoserBuf_LaunchMultiplier(0.0f) == doctest::Approx(1.0f));
    CHECK(LoserBuf_HitMultiplier(1.0f) == doctest::Approx(2.0f));
    CHECK(LoserBuf_LaunchMultiplier(1.0f) == doctest::Approx(1.5f));
}

TEST_CASE("spin lesson assistance begins on roll five and reaches maximum stats")
{
    CHECK(SpinLoserBuf_ValueForRolls(4) == doctest::Approx(0.0f));
    CHECK(SpinLoserBuf_ValueForRolls(5) == doctest::Approx(0.1f));
    CHECK(SpinLoserBuf_ValueForRolls(6) == doctest::Approx(0.1f));
    CHECK(SpinLoserBuf_ValueForRolls(33) == doctest::Approx(1.0f));
    CHECK(SpinLoserBuf_EffectiveStat(0.4f, 0.0f) == doctest::Approx(0.4f));
    CHECK(SpinLoserBuf_EffectiveStat(0.4f, 0.5f) == doctest::Approx(0.7f));
    CHECK(SpinLoserBuf_EffectiveStat(0.4f, 1.0f) == doctest::Approx(1.0f));
    CHECK(SpinLoserBuf_EffectiveStat(0.8f, 1.0f) == doctest::Approx(1.0f));
}
