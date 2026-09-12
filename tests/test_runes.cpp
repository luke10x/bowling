#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../runes.h"
#include "../bolt_logic.h"

TEST_CASE("Rune availability table separates offense and defense runes")
{
    CHECK(Rune_IsEnabledForStage(0, RuneStage::Offense));
    CHECK_FALSE(Rune_IsEnabledForStage(0, RuneStage::Defense));

    CHECK(Rune_IsEnabledForStage(1, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(1, RuneStage::Defense));

    CHECK(Rune_IsEnabledForStage(2, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(2, RuneStage::Defense));

    CHECK(Rune_IsEnabledForStage(3, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(3, RuneStage::Defense));

    CHECK_FALSE(Rune_IsEnabledForStage(4, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(4, RuneStage::Defense));

    CHECK(Rune_IsEnabledForStage(5, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(5, RuneStage::Defense));
}

TEST_CASE("Bolt targets only blocks still ahead of the rolling player ball")
{
    CHECK(BoltShouldTargetBlock(true, true, -14.0f, -10.0f, 1.0f));
    CHECK_FALSE(BoltShouldTargetBlock(true, true, -9.0f, -10.0f, 1.0f));
    CHECK_FALSE(BoltShouldTargetBlock(false, true, -14.0f, -10.0f, 1.0f));

    CHECK(BoltShouldTargetBlock(true, true, -2.0f, -6.0f, -1.0f));
    CHECK_FALSE(BoltShouldTargetBlock(true, true, -7.0f, -6.0f, -1.0f));
    CHECK(BoltShouldTargetBlock(true, false, -7.0f, -6.0f, -1.0f));
}

TEST_CASE("Bolt retargets a destroyed block only while the strike remains active")
{
    CHECK(BoltShouldRetargetPins(true, 0.45f, 1.0f));
    CHECK_FALSE(BoltShouldRetargetPins(true, 1.0f, 1.0f));
    CHECK_FALSE(BoltShouldRetargetPins(false, 0.45f, 1.0f));
}

TEST_CASE("Rolling enemy Bolt destruction distinguishes destroy from escape")
{
    CHECK(BoltResolveRollingBall(false, true) == BoltRollingResolution::Destroy);
    CHECK(BoltResolveRollingBall(true, true) == BoltRollingResolution::Escape);
    CHECK(BoltResolveRollingBall(false, false) == BoltRollingResolution::Escape);
}

TEST_CASE("Both stage allows a rune in either phase")
{
    CHECK(RuneStage_Allows(RuneStage::Both, RuneStage::Offense));
    CHECK(RuneStage_Allows(RuneStage::Both, RuneStage::Defense));
}
