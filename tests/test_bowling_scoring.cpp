#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <vector>

#include "../score.h"
#include "../stubs.h"

static void RollMany(BowlingScoreboard *sb, const std::vector<int> &rolls)
{
    for (int p : rolls)
        (void)addRoll(sb, p);
}

static void RollNineGutterFrames(BowlingScoreboard *sb)
{
    for (int i = 0; i < 9; ++i)
    {
        (void)addRoll(sb, 0);
        (void)addRoll(sb, 0);
    }
}

TEST_CASE("SOLO: 10th frame strike 10,1,1 scores 12")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {10, 1, 1});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].frameScore == 12);
    CHECK(sb.totalScore == 12);
}

TEST_CASE("SOLO: 10th frame strike 10,0,10 scores 20")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {10, 0, 10});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].frameScore == 20);
    CHECK(sb.totalScore == 20);
}

TEST_CASE("SOLO: 10th frame open 5,4 scores 9 (no roll3)")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {5, 4});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].roll3 == -1);
    CHECK(sb.frames[9].frameScore == 9);
    CHECK(sb.totalScore == 9);
}

TEST_CASE("SOLO: 10th frame spare 5,5,10 scores 20")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {5, 5, 10});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].isSpare == 1);
    CHECK(sb.frames[9].frameScore == 20);
    CHECK(sb.totalScore == 20);
}

TEST_CASE("SOLO: 10th frame spare 5,5,9 scores 19")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {5, 5, 9});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].isSpare == 1);
    CHECK(sb.frames[9].frameScore == 19);
    CHECK(sb.totalScore == 19);
}

TEST_CASE("SOLO: 10th frame spare 0,10,10 scores 20")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    RollMany(&sb, {0, 10, 10});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[9].isSpare == 1);
    CHECK(sb.frames[9].frameScore == 20);
    CHECK(sb.totalScore == 20);
}

TEST_CASE("SOLO: 9th strike + 10th 1,1 scores 14 total (12 + 2)")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);

    // Frames 1-8 gutters.
    for (int i = 0; i < 8; ++i)
        RollMany(&sb, {0, 0});

    // 9th strike, 10th open.
    RollMany(&sb, {10, 1, 1});

    CHECK(isGameFinished(&sb) == true);
    CHECK(sb.frames[8].frameScore == 12);
    CHECK(sb.frames[9].frameScore == 2);
    CHECK(sb.totalScore == 14);
}

TEST_CASE("BOT: player and angel scoreboards do not interfere (10th-frame bonuses independent)")
{
    BowlingScoreboard player;
    BowlingScoreboard angel;
    resetScoreboard(&player);
    resetScoreboard(&angel);

    RollNineGutterFrames(&player);
    RollNineGutterFrames(&angel);

    // Player: open 5,4 (total 9).
    RollMany(&player, {5, 4});
    CHECK(isGameFinished(&player) == true);
    CHECK(player.totalScore == 9);

    // Angel: strike + bonus 0,10 (total 20).
    RollMany(&angel, {10, 0, 10});
    CHECK(isGameFinished(&angel) == true);
    CHECK(angel.totalScore == 20);

    // Ensure no crossover.
    CHECK(player.totalScore != angel.totalScore);
    CHECK(player.frames[9].frameScore == 9);
    CHECK(angel.frames[9].frameScore == 20);
}

TEST_CASE("Secret end scoreboard setup leaves only the tenth frame unplayed")
{
    BowlingScoreboard player;
    BowlingScoreboard angel;

    setupStubScoreboardEndOfRound(&player);
    setupStubScoreboardEndOfRound(&angel);

    CHECK(Bowling_ActiveFrameIndex(&player) == 9);
    CHECK(Bowling_ActiveFrameIndex(&angel) == 9);
    CHECK(isGameFinished(&player) == false);
    CHECK(isGameFinished(&angel) == false);
    CHECK(player.totalScore == 63);
    CHECK(angel.totalScore == 63);

    for (int i = 0; i < 9; ++i)
    {
        CHECK(player.frames[i].roll1 == 3);
        CHECK(player.frames[i].roll2 == 4);
        CHECK(angel.frames[i].roll1 == 3);
        CHECK(angel.frames[i].roll2 == 4);
    }

    CHECK(player.frames[9].roll1 == -1);
    CHECK(player.frames[9].roll2 == -1);
    CHECK(player.frames[9].roll3 == -1);
    CHECK(angel.frames[9].roll1 == -1);
    CHECK(angel.frames[9].roll2 == -1);
    CHECK(angel.frames[9].roll3 == -1);
}

TEST_CASE("Early all-down banner treats second roll in an open frame as spare")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);

    CHECK(Bowling_EarlyAllDownBannerKind(&sb) == BOWLING_EARLY_ALL_DOWN_STRIKE);
    addRoll(&sb, 0);
    CHECK(Bowling_EarlyAllDownBannerKind(&sb) == BOWLING_EARLY_ALL_DOWN_SPARE);
}

TEST_CASE("Early all-down banner treats 10th-frame second roll after strike as strike")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);
    addRoll(&sb, 10);

    CHECK(Bowling_EarlyAllDownBannerKind(&sb) == BOWLING_EARLY_ALL_DOWN_STRIKE);
}

TEST_CASE("Split detector recognizes classic split leaves")
{
    const uint16_t sevenTenMask = uint16_t((1u << 6) | (1u << 9));
    const uint16_t fourSixMask = uint16_t((1u << 3) | (1u << 5));
    const uint16_t fourFiveMask = uint16_t((1u << 3) | (1u << 4));
    const uint16_t headStillUpMask = uint16_t((1u << 0) | (1u << 6) | (1u << 9));

    CHECK(Bowling_IsSplitLeave(sevenTenMask));
    CHECK(Bowling_IsSplitLeave(fourSixMask));
    CHECK(!Bowling_IsSplitLeave(fourFiveMask));
    CHECK(!Bowling_IsSplitLeave(headStillUpMask));
}

TEST_CASE("Split markers are recorded for first and second split opportunities in the tenth")
{
    BowlingScoreboard sb;
    resetScoreboard(&sb);
    RollNineGutterFrames(&sb);

    CHECK(Bowling_RecordSplitIfConvertible(&sb, 8, true));
    CHECK(sb.frames[9].splitRoll1 == 1);
    addRoll(&sb, 8);

    addRoll(&sb, 2); // spare to reach roll3 and verify second split slot stays independent path-free

    BowlingScoreboard sb2;
    resetScoreboard(&sb2);
    RollNineGutterFrames(&sb2);
    addRoll(&sb2, 10);
    CHECK(Bowling_RecordSplitIfConvertible(&sb2, 8, true));
    CHECK(sb2.frames[9].splitRoll2 == 1);
}
