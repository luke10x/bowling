#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../runes.h"

TEST_CASE("Rune availability table separates offense and defense runes")
{
    CHECK(Rune_IsEnabledForStage(0, RuneStage::Offense));
    CHECK_FALSE(Rune_IsEnabledForStage(0, RuneStage::Defense));

    CHECK_FALSE(Rune_IsEnabledForStage(1, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(1, RuneStage::Defense));

    CHECK_FALSE(Rune_IsEnabledForStage(2, RuneStage::Offense));
    CHECK(Rune_IsEnabledForStage(2, RuneStage::Defense));
}

TEST_CASE("Both stage allows a rune in either phase")
{
    CHECK(RuneStage_Allows(RuneStage::Both, RuneStage::Offense));
    CHECK(RuneStage_Allows(RuneStage::Both, RuneStage::Defense));
}
