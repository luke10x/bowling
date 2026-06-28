#pragma once

#include <array>
#include <algorithm>
#include <cstdint>

enum CampaignBlockCardType
{
    CAMPAIGN_BLOCK_CARD_WOOD = 0,
    CAMPAIGN_BLOCK_CARD_BRICK = 1,
    CAMPAIGN_BLOCK_CARD_CONCRETE = 2,
    CAMPAIGN_BLOCK_CARD_GLASS = 3,
    CAMPAIGN_BLOCK_CARD_COUNT = 4,
    CAMPAIGN_BLOCK_CARD_NONE = -1,
};

static constexpr int kCampaignBlockCardHandSize = 3;
static constexpr int kCampaignBlockCardQueueSize = 15;

struct CampaignBlockCardSlot
{
    int type = CAMPAIGN_BLOCK_CARD_NONE;
    bool consumed = false;
};

struct CampaignBlockCardDeckState
{
    std::array<CampaignBlockCardSlot, kCampaignBlockCardHandSize> hand = {};
    std::array<int, kCampaignBlockCardQueueSize> queue = {};
    int queueCursor = kCampaignBlockCardQueueSize;
    int currentFrameNumber = 0;
    bool nonGlassSpentThisThrow = false;
};

struct CampaignBlockEnemyCardChoiceContext
{
    int frameNumber = 1;
    int scoreDelta = 0; // enemy score - player score
    bool targetOutOfMana = false;
    bool targetJustScoredStrikeOrSpare = false;
    float remainingDistanceToPinsM = -1.0f;
    float lateGlassRoll01 = 0.0f;
};

inline uint32_t CampaignBlockCards_NextRandom(uint32_t &state)
{
    if (state == 0u)
        state = 0x6d2b79f5u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline int CampaignBlockCards_EnabledMask(bool wood, bool brick, bool concrete, bool glass)
{
    return (wood ? (1 << CAMPAIGN_BLOCK_CARD_WOOD) : 0) |
           (brick ? (1 << CAMPAIGN_BLOCK_CARD_BRICK) : 0) |
           (concrete ? (1 << CAMPAIGN_BLOCK_CARD_CONCRETE) : 0) |
           (glass ? (1 << CAMPAIGN_BLOCK_CARD_GLASS) : 0);
}

inline bool CampaignBlockCards_IsTypeEnabled(int enabledMask, int type)
{
    return type >= 0 && type < CAMPAIGN_BLOCK_CARD_COUNT && ((enabledMask >> type) & 1) != 0;
}

inline bool CampaignBlockCards_ShouldShowHand(int enabledMask)
{
    return CampaignBlockCards_IsTypeEnabled(enabledMask, CAMPAIGN_BLOCK_CARD_GLASS);
}

inline int CampaignBlockCards_IntroTypeForLevel(int levelNumber)
{
    switch (levelNumber)
    {
        case 5: return CAMPAIGN_BLOCK_CARD_GLASS;
        case 8: return CAMPAIGN_BLOCK_CARD_WOOD;
        case 11: return CAMPAIGN_BLOCK_CARD_BRICK;
        case 13: return CAMPAIGN_BLOCK_CARD_CONCRETE;
        default: return CAMPAIGN_BLOCK_CARD_NONE;
    }
}

inline int CampaignBlockCards_WeightedRandomType(uint32_t &rngState, int enabledMask)
{
    static constexpr int kWeights[CAMPAIGN_BLOCK_CARD_COUNT] = {
        17, // wood
        9,  // brick
        5,  // concrete
        69, // glass
    };

    int totalWeight = 0;
    for (int type = 0; type < CAMPAIGN_BLOCK_CARD_COUNT; ++type)
    {
        if (CampaignBlockCards_IsTypeEnabled(enabledMask, type))
            totalWeight += kWeights[type];
    }

    if (totalWeight <= 0)
        return CAMPAIGN_BLOCK_CARD_NONE;

    const uint32_t roll = CampaignBlockCards_NextRandom(rngState) % uint32_t(totalWeight);
    int cursor = int(roll);
    for (int type = 0; type < CAMPAIGN_BLOCK_CARD_COUNT; ++type)
    {
        if (!CampaignBlockCards_IsTypeEnabled(enabledMask, type))
            continue;
        if (cursor < kWeights[type])
            return type;
        cursor -= kWeights[type];
    }

    for (int type = CAMPAIGN_BLOCK_CARD_COUNT - 1; type >= 0; --type)
    {
        if (CampaignBlockCards_IsTypeEnabled(enabledMask, type))
            return type;
    }
    return CAMPAIGN_BLOCK_CARD_NONE;
}

inline void CampaignBlockCards_RefillQueue(CampaignBlockCardDeckState &deck, int enabledMask, uint32_t &rngState)
{
    for (int i = 0; i < kCampaignBlockCardQueueSize; ++i)
        deck.queue[i] = CampaignBlockCards_WeightedRandomType(rngState, enabledMask);
    deck.queueCursor = 0;
}

inline int CampaignBlockCards_DrawNext(CampaignBlockCardDeckState &deck, int enabledMask, uint32_t &rngState)
{
    if (enabledMask == 0)
        return CAMPAIGN_BLOCK_CARD_NONE;
    if (deck.queueCursor >= kCampaignBlockCardQueueSize)
        CampaignBlockCards_RefillQueue(deck, enabledMask, rngState);
    return deck.queue[deck.queueCursor++];
}

inline void CampaignBlockCards_Clear(CampaignBlockCardDeckState &deck)
{
    deck = CampaignBlockCardDeckState{};
}

inline void CampaignBlockCards_ResetThrow(CampaignBlockCardDeckState &deck)
{
    deck.nonGlassSpentThisThrow = false;
}

inline void CampaignBlockCards_DealFrameHand(
    CampaignBlockCardDeckState &deck,
    int frameNumber,
    int enabledMask,
    int requiredIntroType,
    uint32_t &rngState
)
{
    deck.currentFrameNumber = frameNumber;
    deck.nonGlassSpentThisThrow = false;
    for (int i = 0; i < kCampaignBlockCardHandSize; ++i)
    {
        deck.hand[i].type = CampaignBlockCards_DrawNext(deck, enabledMask, rngState);
        deck.hand[i].consumed = false;
    }

    if (CampaignBlockCards_IsTypeEnabled(enabledMask, requiredIntroType))
    {
        bool alreadyPresent = false;
        for (const CampaignBlockCardSlot &slot : deck.hand)
        {
            if (slot.type == requiredIntroType)
            {
                alreadyPresent = true;
                break;
            }
        }
        if (!alreadyPresent)
            deck.hand[0].type = requiredIntroType;
    }
}

inline bool CampaignBlockCards_CanUseSlot(
    const CampaignBlockCardDeckState &deck,
    int slotIndex,
    bool hasActiveBlock
)
{
    if (slotIndex < 0 || slotIndex >= kCampaignBlockCardHandSize)
        return false;
    const CampaignBlockCardSlot &slot = deck.hand[slotIndex];
    if (slot.type == CAMPAIGN_BLOCK_CARD_NONE || slot.consumed || hasActiveBlock)
        return false;
    if (slot.type != CAMPAIGN_BLOCK_CARD_GLASS && deck.nonGlassSpentThisThrow)
        return false;
    return true;
}

inline bool CampaignBlockCards_ConsumeSlot(CampaignBlockCardDeckState &deck, int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= kCampaignBlockCardHandSize)
        return false;
    CampaignBlockCardSlot &slot = deck.hand[slotIndex];
    if (slot.type == CAMPAIGN_BLOCK_CARD_NONE || slot.consumed)
        return false;
    slot.consumed = true;
    if (slot.type != CAMPAIGN_BLOCK_CARD_GLASS)
        deck.nonGlassSpentThisThrow = true;
    return true;
}

inline int CampaignBlockCards_ChooseEnemySlot(
    const CampaignBlockCardDeckState &deck,
    bool hasActiveBlock,
    const CampaignBlockEnemyCardChoiceContext &ctx
)
{
    const bool lateNearPins =
        ctx.remainingDistanceToPinsM >= 0.0f && ctx.remainingDistanceToPinsM <= 3.0f;
    const bool criticalLatePins =
        ctx.remainingDistanceToPinsM >= 0.0f && ctx.remainingDistanceToPinsM <= 2.0f;
    bool hasEligibleNonGlass = false;
    for (int slotIndex = 0; slotIndex < kCampaignBlockCardHandSize; ++slotIndex)
    {
        if (!CampaignBlockCards_CanUseSlot(deck, slotIndex, hasActiveBlock))
            continue;
        const int type = deck.hand[slotIndex].type;
        if (type != CAMPAIGN_BLOCK_CARD_GLASS)
        {
            hasEligibleNonGlass = true;
            break;
        }
    }

    int bestSlot = -1;
    float bestScore = -1000000.0f;
    for (int slotIndex = 0; slotIndex < kCampaignBlockCardHandSize; ++slotIndex)
    {
        if (!CampaignBlockCards_CanUseSlot(deck, slotIndex, hasActiveBlock))
            continue;

        const int type = deck.hand[slotIndex].type;
        if (lateNearPins && type == CAMPAIGN_BLOCK_CARD_GLASS)
        {
            if (hasEligibleNonGlass)
                continue;
            if (criticalLatePins && ctx.lateGlassRoll01 >= 0.5f)
                continue;
        }

        float score = 0.0f;
        switch (type)
        {
            case CAMPAIGN_BLOCK_CARD_GLASS: score = 1.0f; break;
            case CAMPAIGN_BLOCK_CARD_WOOD: score = 2.4f; break;
            case CAMPAIGN_BLOCK_CARD_BRICK: score = 4.0f; break;
            case CAMPAIGN_BLOCK_CARD_CONCRETE: score = 5.5f; break;
            default: break;
        }

        const bool lateFrame = ctx.frameNumber >= 8;
        const bool closeMatch = ctx.scoreDelta <= 10;
        if (ctx.targetJustScoredStrikeOrSpare)
            score += score * 0.7f;
        if (ctx.scoreDelta < 0)
            score += score * 0.35f;
        if (lateFrame && closeMatch)
            score += score * 0.25f;
        if (ctx.targetOutOfMana)
        {
            if (type == CAMPAIGN_BLOCK_CARD_GLASS)
                score += 3.5f;
            else
                score -= 1.5f + 0.3f * score;
        }
        if (lateNearPins && type != CAMPAIGN_BLOCK_CARD_GLASS)
            score += 1.25f + 0.15f * score;

        if (score > bestScore)
        {
            bestScore = score;
            bestSlot = slotIndex;
        }
    }
    return bestSlot;
}

inline const char *CampaignBlockCards_Label(int type)
{
    switch (type)
    {
        case CAMPAIGN_BLOCK_CARD_WOOD: return "WOOD";
        case CAMPAIGN_BLOCK_CARD_BRICK: return "BRICK";
        case CAMPAIGN_BLOCK_CARD_CONCRETE: return "CONCRETE";
        case CAMPAIGN_BLOCK_CARD_GLASS: return "GLASS";
        default: return "---";
    }
}
