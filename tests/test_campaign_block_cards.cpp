#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_block_cards.h"

TEST_CASE("Block card hand stays hidden before glass is enabled")
{
    CHECK(!CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(false, false, false, false)));
    CHECK(!CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(true, false, false, false)));
    CHECK(CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(true, false, false, true)));
}

TEST_CASE("Campaign and post-campaign runs load their own block tool sets")
{
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(4, false) == 0);
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(5, false) ==
          CampaignBlockCards_EnabledMask(false, false, false, true));
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(8, false) ==
          CampaignBlockCards_EnabledMask(true, false, false, true));
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(11, false) ==
          CampaignBlockCards_EnabledMask(true, true, false, true));
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(12, false) ==
          CampaignBlockCards_EnabledMask(true, true, true, true));
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(1, true) ==
          CampaignBlockCards_EnabledMask(true, true, true, true));
    CHECK(CampaignBlockCards_EnabledMaskForCampaignLevel(13, true, /*blocksEnabled=*/false) == 0);
}

TEST_CASE("Level 5 glass reminder stops once glass has been learned")
{
    CHECK_FALSE(CampaignBlockCards_ShouldPromptGlassLesson(4, 3, false, false));
    CHECK_FALSE(CampaignBlockCards_ShouldPromptGlassLesson(5, 2, false, false));
    CHECK(CampaignBlockCards_ShouldPromptGlassLesson(5, 3, false, false));
    CHECK_FALSE(CampaignBlockCards_ShouldPromptGlassLesson(5, 3, true, false));
    CHECK_FALSE(CampaignBlockCards_ShouldPromptGlassLesson(5, 3, false, true));
}

TEST_CASE("Only Levels 11 and 13 give the enemy final-level block weights")
{
    for (int level = 1; level <= 13; ++level)
    {
        CHECK(CampaignBlockCards_EnemyUsesFinalLevelAdvantage(level, /*postgame=*/false) ==
              (level == 11 || level == 13));
        CHECK_FALSE(CampaignBlockCards_EnemyUsesFinalLevelAdvantage(level, /*postgame=*/true));
    }
}

TEST_CASE("Enemy final-level block weights increase non-glass card chances")
{
    const auto normal = CampaignBlockCardWeightProfile::Default;
    const auto advantage = CampaignBlockCardWeightProfile::EnemyFinalLevelAdvantage;
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_GLASS, normal) == 69);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_WOOD, normal) == 17);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_BRICK, normal) == 9);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_CONCRETE, normal) == 5);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_GLASS, advantage) == 60);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_WOOD, advantage) == 20);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_BRICK, advantage) == 12);
    CHECK(CampaignBlockCards_WeightForType(CAMPAIGN_BLOCK_CARD_CONCRETE, advantage) == 8);

    const int level11Mask = CampaignBlockCards_EnabledMask(true, true, false, true);
    const int level13Mask = CampaignBlockCards_EnabledMask(true, true, true, true);
    CHECK(CampaignBlockCards_TotalEnabledWeight(level11Mask, normal) == 95);
    CHECK(CampaignBlockCards_TotalEnabledWeight(level11Mask, advantage) == 92);
    CHECK(CampaignBlockCards_TotalEnabledWeight(level13Mask, normal) == 100);
    CHECK(CampaignBlockCards_TotalEnabledWeight(level13Mask, advantage) == 100);
}

TEST_CASE("Block card dealing only uses enabled variants")
{
    CampaignBlockCardDeckState deck = {};
    uint32_t rng = 12345u;
    const int enabledMask = CampaignBlockCards_EnabledMask(true, false, false, true);

    CampaignBlockCards_RefillQueue(deck, enabledMask, rng);
    CampaignBlockCards_DealFrameHand(deck, 1, enabledMask, CAMPAIGN_BLOCK_CARD_NONE, rng);

    for (const CampaignBlockCardSlot &slot : deck.hand)
    {
        CHECK((slot.type == CAMPAIGN_BLOCK_CARD_WOOD || slot.type == CAMPAIGN_BLOCK_CARD_GLASS));
        CHECK(!slot.consumed);
    }
}

TEST_CASE("Intro levels guarantee the newly introduced card in the opening hand")
{
    struct IntroCase
    {
        int levelNumber;
        int enabledMask;
        int requiredType;
    };
    const IntroCase cases[] = {
        {5, CampaignBlockCards_EnabledMask(false, false, false, true), CAMPAIGN_BLOCK_CARD_GLASS},
        {8, CampaignBlockCards_EnabledMask(true, false, false, true), CAMPAIGN_BLOCK_CARD_WOOD},
        {11, CampaignBlockCards_EnabledMask(true, true, false, true), CAMPAIGN_BLOCK_CARD_BRICK},
        {13, CampaignBlockCards_EnabledMask(true, true, true, true), CAMPAIGN_BLOCK_CARD_CONCRETE},
    };

    for (const IntroCase &it : cases)
    {
        CampaignBlockCardDeckState deck = {};
        uint32_t rng = 777u;
        CampaignBlockCards_DealFrameHand(
            deck,
            1,
            it.enabledMask,
            CampaignBlockCards_IntroTypeForLevel(it.levelNumber),
            rng
        );

        bool foundRequired = false;
        for (const CampaignBlockCardSlot &slot : deck.hand)
            foundRequired |= (slot.type == it.requiredType);
        CHECK(foundRequired);
    }
}

TEST_CASE("Non-glass cards are limited to one use per throw")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_WOOD;
    deck.hand[1].type = CAMPAIGN_BLOCK_CARD_BRICK;
    deck.hand[2].type = CAMPAIGN_BLOCK_CARD_GLASS;

    CHECK(CampaignBlockCards_CanUseSlot(deck, 0, false));
    CHECK(CampaignBlockCards_ConsumeSlot(deck, 0));
    CHECK(deck.nonGlassSpentThisThrow);
    CHECK(!CampaignBlockCards_CanUseSlot(deck, 1, false));
    CHECK(CampaignBlockCards_CanUseSlot(deck, 2, false));

    CampaignBlockCards_ResetThrow(deck);
    CHECK(!deck.nonGlassSpentThisThrow);
    CHECK(CampaignBlockCards_CanUseSlot(deck, 1, false));
}

TEST_CASE("Multiple glass cards can be used in the same throw after the previous glass is gone")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_GLASS;
    deck.hand[1].type = CAMPAIGN_BLOCK_CARD_GLASS;
    deck.hand[2].type = CAMPAIGN_BLOCK_CARD_WOOD;

    CHECK(CampaignBlockCards_CanUseSlot(deck, 0, false));
    CHECK(CampaignBlockCards_ConsumeSlot(deck, 0));
    CHECK(!CampaignBlockCards_CanUseSlot(deck, 1, true));
    CHECK(CampaignBlockCards_CanUseSlot(deck, 1, false));
}

TEST_CASE("Block card queue refills after fifteen draws")
{
    CampaignBlockCardDeckState deck = {};
    uint32_t rng = 99u;
    const int enabledMask = CampaignBlockCards_EnabledMask(true, true, true, true);

    for (int i = 0; i < 20; ++i)
    {
        const int type = CampaignBlockCards_DrawNext(deck, enabledMask, rng);
        CHECK(type >= CAMPAIGN_BLOCK_CARD_WOOD);
        CHECK(type <= CAMPAIGN_BLOCK_CARD_GLASS);
    }

    CHECK(deck.queueCursor > 0);
    CHECK(deck.queueCursor <= kCampaignBlockCardQueueSize);
}

TEST_CASE("Enemy card chooser prefers glass when the player is out of mana")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_WOOD;
    deck.hand[1].type = CAMPAIGN_BLOCK_CARD_BRICK;
    deck.hand[2].type = CAMPAIGN_BLOCK_CARD_GLASS;

    CampaignBlockEnemyCardChoiceContext ctx = {};
    ctx.frameNumber = 6;
    ctx.scoreDelta = 0;
    ctx.targetOutOfMana = true;

    CHECK(CampaignBlockCards_ChooseEnemySlot(deck, false, ctx) == 2);
}

TEST_CASE("Enemy card chooser prefers stronger cards after a strike or spare")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_WOOD;
    deck.hand[1].type = CAMPAIGN_BLOCK_CARD_CONCRETE;
    deck.hand[2].type = CAMPAIGN_BLOCK_CARD_GLASS;

    CampaignBlockEnemyCardChoiceContext ctx = {};
    ctx.frameNumber = 9;
    ctx.scoreDelta = -8;
    ctx.targetJustScoredStrikeOrSpare = true;

    CHECK(CampaignBlockCards_ChooseEnemySlot(deck, false, ctx) == 1);
}

TEST_CASE("Enemy card chooser avoids glass near pins when stronger late block cards are available")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_WOOD;
    deck.hand[1].type = CAMPAIGN_BLOCK_CARD_BRICK;
    deck.hand[2].type = CAMPAIGN_BLOCK_CARD_GLASS;

    CampaignBlockEnemyCardChoiceContext ctx = {};
    ctx.frameNumber = 8;
    ctx.scoreDelta = 0;
    ctx.remainingDistanceToPinsM = 2.4f;
    ctx.lateGlassRoll01 = 0.9f;

    CHECK(CampaignBlockCards_ChooseEnemySlot(deck, false, ctx) == 1);
}

TEST_CASE("Enemy card chooser makes deterministic 50 50 late glass choice when only glass is left")
{
    CampaignBlockCardDeckState deck = {};
    deck.hand[0].type = CAMPAIGN_BLOCK_CARD_GLASS;

    CampaignBlockEnemyCardChoiceContext reactCtx = {};
    reactCtx.remainingDistanceToPinsM = 1.8f;
    reactCtx.lateGlassRoll01 = 0.2f;
    CHECK(CampaignBlockCards_ChooseEnemySlot(deck, false, reactCtx) == 0);

    CampaignBlockEnemyCardChoiceContext failCtx = {};
    failCtx.remainingDistanceToPinsM = 1.8f;
    failCtx.lateGlassRoll01 = 0.8f;
    CHECK(CampaignBlockCards_ChooseEnemySlot(deck, false, failCtx) == -1);
}
