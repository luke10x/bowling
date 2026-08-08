#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#define CHEST_RENDER_NO_GPU
#include "../chest.h"

TEST_CASE("Chest reward close remains visible without restoring black cinematic overlay")
{
    using Phase = ChestRender::CollectiblePhase;

    CHECK(ChestRender::IsRewardActive(Phase::RewardClosing));
    CHECK(ChestRender::IsTextureActive(Phase::RewardClosing));
    CHECK_FALSE(ChestRender::IsCinematicActive(Phase::RewardClosing));
    CHECK(ChestRender::CinematicOverlayAlpha(Phase::RewardClosing) == 0);
}

TEST_CASE("Chest payout phases keep texture alive until spin-out finishes")
{
    using Phase = ChestRender::CollectiblePhase;

    CHECK(ChestRender::IsTextureActive(Phase::Payout));
    CHECK(ChestRender::IsTextureActive(Phase::RewardClosing));
    CHECK(ChestRender::IsTextureActive(Phase::RewardSpinOut));
    CHECK_FALSE(ChestRender::IsTextureActive(Phase::Disabled));
}

TEST_CASE("Chest black overlay is limited to pre-open cinematic phases")
{
    using Phase = ChestRender::CollectiblePhase;

    CHECK(ChestRender::IsCinematicActive(Phase::CollectedMove));
    CHECK(ChestRender::IsCinematicActive(Phase::WaitingTap));
    CHECK_FALSE(ChestRender::IsCinematicActive(Phase::Aligning));
    CHECK_FALSE(ChestRender::IsCinematicActive(Phase::Opening));
    CHECK_FALSE(ChestRender::IsCinematicActive(Phase::Payout));
}
