#pragma once

#include <string.h>

#include "../shop.h" // CAROUSEL_MAX_CARDS

// Keep this independent from game.cpp (no dependency on BotAvatar enum there).
enum BotCatalogAvatarKind
{
    BotCatalogAvatar_ANGEL = 0,
    BotCatalogAvatar_CHERUB = 1,
    BotCatalogAvatar_SERAPH = 2,
    BotCatalogAvatar_THRONE = 3,
};

typedef struct
{
    const char *name;
    const char *rarity;
    BotCatalogAvatarKind kind;
} BotCatalogItem;

static const BotCatalogItem g_botCatalog[] = {
    {.name = "Angel", .rarity = "RARE", .kind = BotCatalogAvatar_ANGEL},
    {.name = "Cherub", .rarity = "EPIC", .kind = BotCatalogAvatar_CHERUB},
    {.name = "Seraph", .rarity = "LEGENDARY", .kind = BotCatalogAvatar_SERAPH},
    {.name = "Throne", .rarity = "LEGENDARY", .kind = BotCatalogAvatar_THRONE},
};

static const int g_botCatalogCount = (int)(sizeof(g_botCatalog) / sizeof(g_botCatalog[0]));

// Mirrors HouseCarouselState, but for bot avatars (3 cards).
typedef struct
{
    bool isGrabbed;
    int startingX;
    float scrollOffset;
    float velocity;
    bool isDragging;
    bool isAutoDragging;

    int closestBotIdx;
    int closest2ndBotIdx;
    int closest3rdBotIdx;
    BotCatalogItem items[CAROUSEL_MAX_CARDS];
    int cardCount;
} BotCarouselState;

inline void BotCarousel_Init(BotCarouselState *cs)
{
    memset(cs, 0, sizeof(BotCarouselState));
    cs->velocity = 0.0f;
    cs->isGrabbed = false;
    cs->startingX = 0;
    cs->closestBotIdx = -1;
    cs->closest2ndBotIdx = -1;
    cs->closest3rdBotIdx = -1;
}

inline bool BotCarousel_Add(BotCarouselState *cs, BotCatalogAvatarKind kind)
{
    if (!cs || cs->cardCount >= CAROUSEL_MAX_CARDS)
        return false;
    for (int i = 0; i < g_botCatalogCount; i++)
    {
        if (g_botCatalog[i].kind == kind)
        {
            memcpy(&cs->items[cs->cardCount], &g_botCatalog[i], sizeof(BotCatalogItem));
            cs->cardCount++;
            return true;
        }
    }
    return false;
}

inline void BotCarousel_SetupDefault(BotCarouselState *cs)
{
    if (!cs)
        return;
    cs->cardCount = 0;
    cs->scrollOffset = 0.0f;
    cs->velocity = 0.0f;
    cs->isDragging = false;
    cs->isAutoDragging = false;
    cs->isGrabbed = false;
    cs->startingX = 0;

    BotCarousel_Add(cs, BotCatalogAvatar_ANGEL);
    BotCarousel_Add(cs, BotCatalogAvatar_CHERUB);
    BotCarousel_Add(cs, BotCatalogAvatar_SERAPH);
    BotCarousel_Add(cs, BotCatalogAvatar_THRONE);
}
