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

TEST_CASE("Chest prize selection can exclude boom rewards")
{
    CHECK(ChestRender::SelectPrizeForLevel(5, 0.00f, false) != ChestRender::PrizeKind::RuneBoom);
    CHECK(ChestRender::SelectPrizeForLevel(5, 0.61f, false) != ChestRender::PrizeKind::RuneBoom);
    CHECK(ChestRender::SelectPrizeForLevel(5, 0.99f, false) != ChestRender::PrizeKind::RuneBoom);
}

TEST_CASE("Chest boom prize inventory gate requires spare balls and no carried boom rune")
{
    CHECK(ChestRender::AllowBoomPrizeForInventory(1, 0) == false);
    CHECK(ChestRender::AllowBoomPrizeForInventory(2, 1) == false);
    CHECK(ChestRender::AllowBoomPrizeForInventory(3, 2) == false);
    CHECK(ChestRender::AllowBoomPrizeForInventory(2, 0) == true);
    CHECK(ChestRender::AllowBoomPrizeForInventory(4, 0) == true);
}

TEST_CASE("Chest prize selection includes newer rune awards")
{
    CHECK(ChestRender::SelectPrizeForLevel(11, 0.62f) == ChestRender::PrizeKind::RuneSkull);
    CHECK(ChestRender::SelectPrizeForLevel(11, 0.86f) == ChestRender::PrizeKind::RuneGuardPins);
    CHECK(ChestRender::SelectPrizeForLevel(13, 0.95f) == ChestRender::PrizeKind::RuneFootball);
}
