#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../campaign_block_cards.h"

TEST_CASE("Block card hand stays hidden before glass is enabled")
{
    CHECK(!CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(false, false, false, false)));
    CHECK(!CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(true, false, false, false)));
    CHECK(CampaignBlockCards_ShouldShowHand(CampaignBlockCards_EnabledMask(true, false, false, true)));
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
