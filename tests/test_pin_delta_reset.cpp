#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../bowling/pin_delta.h"

TEST_CASE("Rack reset clears wereDead so 10th-frame roll3 delta is not negative")
{
    int wereDead = 0;

    // 10th frame: roll1 knocks 9 (cumulative down = 9).
    CHECK(Bowling_ComputeKnockedThisRoll(9, wereDead) == 9);
    Bowling_AdvanceWereDead(9, &wereDead); // wereDead becomes 9
    CHECK(wereDead == 9);

    // roll2 knocks 1 (spare; cumulative down = 10).
    CHECK(Bowling_ComputeKnockedThisRoll(10, wereDead) == 1);
    Bowling_AdvanceWereDead(10, &wereDead); // wereDead becomes 19
    CHECK(wereDead == 19);

    // Fresh rack for roll3 must reset wereDead to 0.
    Bowling_OnRackReset(&wereDead);
    CHECK(wereDead == 0);

    // roll3 knocks 7 from a fresh rack (cumulative down = 7).
    CHECK(Bowling_ComputeKnockedThisRoll(7, wereDead) == 7);
}

TEST_CASE("Missing rack reset reproduces the regression (negative knockedThisRoll)")
{
    int wereDead = 0;
    Bowling_AdvanceWereDead(9, &wereDead);  // after roll1 (cumulative 9)
    Bowling_AdvanceWereDead(10, &wereDead); // after roll2 (cumulative 10)
    CHECK(wereDead == 19);

    // Now a fresh rack happens but wereDead wasn't cleared: delta goes negative.
    CHECK(Bowling_ComputeKnockedThisRoll(7, wereDead) < 0);
}

