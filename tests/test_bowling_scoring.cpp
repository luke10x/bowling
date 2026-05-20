#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include <vector>

#include "../score.h"

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
