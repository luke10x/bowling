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

static Clay_RenderCommandArray RenderResultFixture(Clayton *ui)
{
    static void *memory = std::malloc(Clay_MinMemorySize());
    Clay_Initialize(Clay_CreateArenaWithCapacityAndMemory(Clay_MinMemorySize(), memory),
        {430, 932}, {});
    Clay_SetMeasureTextFunction([](Clay_StringSlice text, Clay_TextElementConfig *cfg, void *) {
        return Clay_Dimensions{float(text.length) * cfg->fontSize * 0.6f, cfg->fontSize * 1.2f};
    }, nullptr);
    ClayArena_Init(&ui->clayArena);
    initClaytonClick(&ui->playAgainClick, "fixtureContinue");
    initClaytonClick(&ui->resultRepeatClick, "fixtureRepeat");
    initClaytonClick(&ui->newGameShopClick, "fixtureShop");
    Clay_BeginLayout();
    CLAY(CLAY_ID("Root"), {.layout = {.sizing = {CLAY_SIZING_FIXED(430), CLAY_SIZING_FIXED(932)}}})
    { renderNewGameWindow(ui); }
    return Clay_EndLayout();
}

static bool ResultContainsText(const Clay_RenderCommandArray &commands, const char *expected)
{
    for (int i = 0; i < commands.length; ++i)
    {
        const auto &cmd = commands.internalArray[i];
        if (cmd.commandType == CLAY_RENDER_COMMAND_TYPE_TEXT)
        {
            auto text = cmd.renderData.text.stringContents;
            if (std::string(text.chars, text.length) == expected) return true;
        }
    }
    return false;
}

TEST_CASE("Bowling and bonus results show one currency total, with bowling scoreboards below the totals")
{
    // Clayton embeds its text arena, so allocate it outside the wasm stack.
    auto *ui = new Clayton{};
    ui->newGameIsResult = true;
    ui->newGameShowMoneyBreakdown = true;
    ui->newGameRoundCoins = 5;
    ui->newGameRoundMoneyTotal = 5;
    std::snprintf(ui->newGameDurationText, sizeof(ui->newGameDurationText), "4:37");
    ui->newGameTitle = "VICTORY";
    ui->newGameButtonLabel = "NEXT";
    SUBCASE("Bowling")
    {
        ui->newGameShowScores = true;
        ui->newGameShowOpponent = true;
        const auto commands = RenderResultFixture(ui);
        const auto totals = Clay_GetElementData(CLAY_ID("NewGameScoreDuel"));
        const auto boards = Clay_GetElementData(CLAY_ID("ResultScoreboards"));
        CHECK(boards.found);
        CHECK(boards.boundingBox.y >= totals.boundingBox.y + totals.boundingBox.height);
        CHECK_FALSE(Clay_GetElementData(CLAY_ID("ResultScoreboardFrames")).found);
        const auto first = Clay_GetElementData(CLAY_IDI("ResultThrowSlot", 0)).boundingBox;
        const auto second = Clay_GetElementData(CLAY_IDI("ResultThrowSlot", 1)).boundingBox;
        CHECK(second.x >= first.x + first.width);
        CHECK(first.width == doctest::Approx(second.width));
        CHECK_FALSE(Clay_GetElementData(CLAY_ID("NewGameCoinsPanel")).found);
        CHECK(ResultContainsText(commands, "Total Time  4:37"));
        CHECK(ResultContainsText(commands, "Total"));
        CHECK(ResultContainsText(commands, "$ 5"));
        CHECK_FALSE(ResultContainsText(commands, "COINS OBTAINED"));
    }
    SUBCASE("Minigame")
    {
        const auto commands = RenderResultFixture(ui);
        CHECK_FALSE(Clay_GetElementData(CLAY_ID("ResultScoreboards")).found);
        CHECK_FALSE(Clay_GetElementData(CLAY_ID("NewGameCoinsPanel")).found);
        CHECK(ResultContainsText(commands, "Total Time  4:37"));
        CHECK(ResultContainsText(commands, "Total"));
        CHECK(ResultContainsText(commands, "$ 5"));
        CHECK_FALSE(ResultContainsText(commands, "COINS OBTAINED"));
    }
    delete ui;
}

TEST_CASE("Post-campaign results identify the finished match's N B R tool settings")
{
    auto *ui = new Clayton{};
    ui->newGameIsResult = true;
    ui->newGamePostCampaign = true;
    ui->newGameContextLabel = "POST-CAMPAIGN";
    ui->newGameNosEnabled = true;
    ui->newGameBlocksEnabled = false;
    ui->newGameRunesEnabled = true;
    const auto commands = RenderResultFixture(ui);
    CHECK(ResultContainsText(commands, "POST-CAMPAIGN"));
    CHECK(ResultContainsText(commands, "N +"));
    CHECK(ResultContainsText(commands, "B -"));
    CHECK(ResultContainsText(commands, "R +"));
    delete ui;
}

TEST_CASE("Pixel snapping preserves every one-pixel scoreboard divider")
{
    for (float width : {31.2f, 37.6f, 42.8f})
    {
        for (int offset = 0; offset < 10; ++offset)
        {
            for (int frame = 0; frame < 9; ++frame)
            {
                const float x = offset * 0.1f + frame * (width + 1.0f);
                const auto cell = Gles3_SnapBoundingBox({x, 0.6f, width, 39.6f});
                const auto next = Gles3_SnapBoundingBox({x + width + 1.0f, 0.6f, width, 39.6f});
                CHECK(next.x - (cell.x + cell.width) == doctest::Approx(1.0f));
                CHECK(cell.y + cell.height == doctest::Approx(std::round(0.6f + 39.6f)));
            }
        }
    }
}
