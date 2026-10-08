#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../lane_pushback.h"

TEST_CASE("Lane pushback only affects balls supported by the playable lane")
{
    const float edge = LanePushback::PLAYABLE_HALF_WIDTH_M;

    CHECK(LanePushback::IsBallOnPlayableLane(0.0f, 0.11f));
    CHECK(LanePushback::IsBallOnPlayableLane(edge, 0.11f));

    // A gutter ball must not receive a force back toward the lane while falling.
    CHECK_FALSE(LanePushback::IsBallOnPlayableLane(edge + 0.001f, 0.11f));
    CHECK_FALSE(LanePushback::IsBallOnPlayableLane(-edge - 0.001f, 0.11f));
    CHECK_FALSE(LanePushback::IsBallOnPlayableLane(0.0f, LanePushback::MAX_SURFACE_CENTER_Y_M));
}
