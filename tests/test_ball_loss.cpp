#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../ball_loss.h"

TEST_CASE("Destroyed player ball is removed and marked for possible win return")
{
    uint64_t owned = (1ull << 0) | (1ull << 5);
    uint64_t pending = 0ull;

    CHECK(BallLoss::RemoveDestroyedPlayerBall(&owned, &pending, 5));
    CHECK((owned & (1ull << 5)) == 0ull);
    CHECK((owned & (1ull << 0)) != 0ull);
    CHECK((pending & (1ull << 5)) != 0ull);
}

TEST_CASE("Winning restores destroyed balls and clears pending return")
{
    uint64_t owned = (1ull << 0);
    uint64_t pending = (1ull << 5) | (1ull << 10);

    const uint64_t restored = BallLoss::RestorePendingDestroyedBallsOnWin(&owned, &pending, true);

    CHECK(restored == ((1ull << 5) | (1ull << 10)));
    CHECK((owned & (1ull << 5)) != 0ull);
    CHECK((owned & (1ull << 10)) != 0ull);
    CHECK(pending == 0ull);
}

TEST_CASE("Losing does not restore destroyed balls")
{
    uint64_t owned = (1ull << 0);
    uint64_t pending = (1ull << 5);

    CHECK(BallLoss::RestorePendingDestroyedBallsOnWin(&owned, &pending, false) == 0ull);
    CHECK((owned & (1ull << 5)) == 0ull);
    CHECK(pending == (1ull << 5));

    BallLoss::ClearPendingDestroyedBalls(&pending);
    CHECK(pending == 0ull);
}

TEST_CASE("Invalid or unowned destroyed balls are ignored")
{
    uint64_t owned = (1ull << 0);
    uint64_t pending = 0ull;

    CHECK_FALSE(BallLoss::RemoveDestroyedPlayerBall(&owned, &pending, 5));
    CHECK_FALSE(BallLoss::RemoveDestroyedPlayerBall(&owned, &pending, 63));
    CHECK(owned == (1ull << 0));
    CHECK(pending == 0ull);
}
