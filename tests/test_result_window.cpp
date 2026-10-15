#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../clayton/win_stack.h"

TEST_CASE("Bare Retry prompts continue through the existing button action without a window")
{
    WindowStack stack{};
    stack.windowStackInit();
    stack.windowStackPushNewGameWindow(/*hasResults=*/false);

    CHECK(stack.playAgainRequested);
    CHECK(stack.count == 0);
    CHECK_FALSE(stack.resultRepeatRequested);
    CHECK_FALSE(stack.resultNextRequested);
}

TEST_CASE("Score and reward results remain visible until the player chooses an action")
{
    WindowStack stack{};
    stack.windowStackInit();
    stack.windowStackPushNewGameWindow(/*hasResults=*/true);

    CHECK_FALSE(stack.playAgainRequested);
    REQUIRE(stack.count == 1);
    CHECK(stack.kinds[stack.count - 1] == WindowKind_NewGame);
    // Repeated RESULT ticks keep a single window and never auto-accept it.
    stack.windowStackPushNewGameWindow(/*hasResults=*/true);
    CHECK(stack.count == 1);
    CHECK_FALSE(stack.playAgainRequested);
}

TEST_CASE("Skipping an empty Retry prompt preserves other pending windows and actions")
{
    WindowStack stack{};
    stack.windowStackInit();
    stack.windowStackPushLocalHiscoreWindow();
    stack.bonusPlayRequested = true;
    stack.windowStackPushNewGameWindow(/*hasResults=*/false);

    CHECK(stack.playAgainRequested);
    CHECK(stack.bonusPlayRequested);
    REQUIRE(stack.count == 1);
    CHECK(stack.kinds[stack.count - 1] == WindowKind_LocalHiscore);
}
