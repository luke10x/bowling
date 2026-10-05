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
    CHECK(ChestRender::SelectPrizeForLevel(11, 0.67f) == ChestRender::PrizeKind::RuneSkull);
    CHECK(ChestRender::SelectPrizeForLevel(11, 0.90f) == ChestRender::PrizeKind::RuneGuardPins);
    CHECK(ChestRender::SelectPrizeForLevel(13, 0.95f) == ChestRender::PrizeKind::RuneFootball);
}

TEST_CASE("Post-campaign chest configuration explicitly references Level 12")
{
    CHECK(ChestRender::kPostCampaignReferenceLevel == 12);
    const ChestRender::SpawnChanceConfig chance =
        ChestRender::SpawnChanceForLevel(ChestRender::kPostCampaignReferenceLevel);
    CHECK(chance.numerator == 1);
    CHECK(chance.denominator == 7);
    CHECK(ChestRender::SelectPrizeForLevel(
              ChestRender::kPostCampaignReferenceLevel, 0.99f) ==
          ChestRender::PrizeKind::RuneFootball);
}

TEST_CASE("Chest rune chance halves for every carried rune above four")
{
    CHECK(ChestRender::RunePrizeChanceMultiplierForCarriedRunes(0) == doctest::Approx(1.0f));
    CHECK(ChestRender::RunePrizeChanceMultiplierForCarriedRunes(4) == doctest::Approx(1.0f));
    CHECK(ChestRender::RunePrizeChanceMultiplierForCarriedRunes(5) == doctest::Approx(0.5f));
    CHECK(ChestRender::RunePrizeChanceMultiplierForCarriedRunes(6) == doctest::Approx(0.25f));
    CHECK(ChestRender::RunePrizeChanceMultiplierForCarriedRunes(7) == doctest::Approx(0.125f));
}

TEST_CASE("Chest rune hoarding penalty reduces total rune odds and recovers after spending")
{
    // Level 13's base rewards are 90% runes and 10% money. At five carried
    // runes the rune chance is exactly halved to 45%; at six it is 22.5%.
    CHECK(ChestRender::PrizeIsRune(ChestRender::SelectPrizeForLevel(13, 0.449f, true, 0.5f)));
    CHECK_FALSE(ChestRender::PrizeIsRune(ChestRender::SelectPrizeForLevel(13, 0.451f, true, 0.5f)));
    CHECK(ChestRender::PrizeIsRune(ChestRender::SelectPrizeForLevel(13, 0.224f, true, 0.25f)));
    CHECK_FALSE(ChestRender::PrizeIsRune(ChestRender::SelectPrizeForLevel(13, 0.226f, true, 0.25f)));

    // Re-evaluating at four carried runes uses the unmodified base table again.
    CHECK(ChestRender::SelectPrizeForLevel(13, 0.95f, true,
                                            ChestRender::RunePrizeChanceMultiplierForCarriedRunes(4)) ==
          ChestRender::PrizeKind::RuneFootball);
}
