#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../score.h"

static void RollOpenFrames(BowlingScoreboard *sb, int pins1, int pins2)
{
    for (int i = 0; i < 10; i++)
    {
        addRoll(sb, pins1);
        addRoll(sb, pins2);
    }
}

TEST_CASE("BOT mode: player and Angel scoreboards are independent")
{
    BowlingScoreboard player = {};
    BowlingScoreboard angel = {};
    resetScoreboard(&player);
    resetScoreboard(&angel);

    // Player bowls one roll.
    addRoll(&player, 7);

    CHECK(player.frames[0].roll1 == 7);
    CHECK(player.totalScore >= 0);

    // Angel board must be untouched.
    CHECK(angel.frames[0].roll1 == -1);
    CHECK(angel.totalScore == 0);
}

TEST_CASE("BOT mode: winner rule is strict (tie counts as loss)")
{
    BowlingScoreboard player = {};
    BowlingScoreboard angel = {};
    resetScoreboard(&player);
    resetScoreboard(&angel);

    // Player: 9-miss every frame => 90.
    RollOpenFrames(&player, 9, 0);
    // Angel: 8-miss every frame => 80.
    RollOpenFrames(&angel, 8, 0);

    CHECK(isGameFinished(&player));
    CHECK(isGameFinished(&angel));
    CHECK(player.totalScore == 90);
    CHECK(angel.totalScore == 80);

    const bool playerWins = (player.totalScore > angel.totalScore);
    CHECK(playerWins == true);

    // Tie: both 90 => player must NOT win.
    resetScoreboard(&player);
    resetScoreboard(&angel);
    RollOpenFrames(&player, 9, 0);
    RollOpenFrames(&angel, 9, 0);
    CHECK(player.totalScore == 90);
    CHECK(angel.totalScore == 90);
    CHECK((player.totalScore > angel.totalScore) == false);
}

TEST_CASE("Scoring: spare bonus applies to the correct board only")
{
    BowlingScoreboard player = {};
    BowlingScoreboard angel = {};
    resetScoreboard(&player);
    resetScoreboard(&angel);

    // Player: spare (5,5) then 7,0 then all gutters.
    addRoll(&player, 5);
    addRoll(&player, 5); // spare
    addRoll(&player, 7); // next roll bonus for spare
    addRoll(&player, 0);
    for (int i = 0; i < 8; i++)
    {
        addRoll(&player, 0);
        addRoll(&player, 0);
    }

    // Angel: all gutters.
    RollOpenFrames(&angel, 0, 0);

    CHECK(player.totalScore == 24); // Frame1: 10+7, Frame2: 7, rest: 0
    CHECK(angel.totalScore == 0);
}
