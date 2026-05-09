#pragma once

#include <string.h>
#include "../shop.h"

// Houses use the same carousel interaction model as the shop, but select lane "houses".
// Keep this file independent from game.cpp UserContext.

typedef struct
{
    int id;
    char name[20];
    char rarity[60];
    char theme[20];

    // Lane params for this house
    float laneFriction;
    float lanePushbackStrength;
    float laneOilThickness;
    float leftOilFadeStartM;
    float leftOilFadeEndM;
    float rightOilFadeStartM;
    float rightOilFadeEndM;
    float oilCarrydownPerBallTravelM;
    float oilThicknessDecayPerBallTravel;

    // Which lane background variant to use (0..3).
    // The lane mesh UVs are authored in 1/8 steps within the atlas; selecting a variant is a V offset by N*(1/8).
    // 0 is the default lane texture and must match "neutral" shader params.
    int laneTextureIdx;

    // Which pin color variant to use (0..3).
    // Pins are authored in the atlas column 0; selection uses the same 1/8 V stepping as lane variants.
    int pinTextureIdx;
} HouseCatalogItem;

static const HouseCatalogItem g_houseCatalog[] = {
    {.id = 0,
     .name = "Classic House",
     .rarity = "COMMON",
     .theme = "House",
     .laneFriction = 0.05f,
     .lanePushbackStrength = 18.0f,
     .laneOilThickness = 1.0f,
     .leftOilFadeStartM = 8.3f,
     .leftOilFadeEndM = 13.3f,
     .rightOilFadeStartM = 8.3f,
     .rightOilFadeEndM = 13.3f,
	     .oilCarrydownPerBallTravelM = 0.01f,
	     .oilThicknessDecayPerBallTravel = 0.001f,
	     .laneTextureIdx = 0,
	     .pinTextureIdx = 0},
    {.id = 1,
     .name = "Dry Fronts",
     .rarity = "RARE",
     .theme = "House",
     .laneFriction = 0.07f,
     .lanePushbackStrength = 10.0f,
     .laneOilThickness = 0.75f,
     .leftOilFadeStartM = 6.8f,
     .leftOilFadeEndM = 11.5f,
     .rightOilFadeStartM = 6.8f,
     .rightOilFadeEndM = 11.5f,
	     .oilCarrydownPerBallTravelM = 0.008f,
	     .oilThicknessDecayPerBallTravel = 0.0015f,
	     .laneTextureIdx = 1,
	     .pinTextureIdx = 1},
    {.id = 2,
     .name = "Long Oil",
     .rarity = "EPIC",
     .theme = "House",
     .laneFriction = 0.04f,
     .lanePushbackStrength = 22.0f,
     .laneOilThickness = 1.0f,
     .leftOilFadeStartM = 10.0f,
     .leftOilFadeEndM = 15.5f,
     .rightOilFadeStartM = 10.0f,
     .rightOilFadeEndM = 15.5f,
	     .oilCarrydownPerBallTravelM = 0.012f,
	     .oilThicknessDecayPerBallTravel = 0.0009f,
	     .laneTextureIdx = 2,
	     .pinTextureIdx = 2},
    {.id = 3,
     .name = "Asym Split",
     .rarity = "LEGENDARY",
     .theme = "House",
     .laneFriction = 0.055f,
     .lanePushbackStrength = 28.0f,
     .laneOilThickness = 0.9f,
     .leftOilFadeStartM = 7.2f,
     .leftOilFadeEndM = 12.8f,
     .rightOilFadeStartM = 9.0f,
     .rightOilFadeEndM = 14.2f,
	     .oilCarrydownPerBallTravelM = 0.011f,
	     .oilThicknessDecayPerBallTravel = 0.0012f,
	     .laneTextureIdx = 3,
	     .pinTextureIdx = 3},
};

static const int g_houseCatalogCount = (int)(sizeof(g_houseCatalog) / sizeof(g_houseCatalog[0]));

typedef struct
{
    bool isGrabbed;
    int startingX;
    float scrollOffset;
    float velocity;
    bool isDragging;
    bool isAutoDragging;

    int closestHouseIdx;
    int closest2ndHouseIdx;
    int closest3rdHouseIdx;
    HouseCatalogItem items[CAROUSEL_MAX_CARDS];
    int cardCount;
} HouseCarouselState;

inline void HouseCarousel_Init(HouseCarouselState *cs)
{
    memset(cs, 0, sizeof(HouseCarouselState));
    cs->velocity = 0.0f;
    cs->isGrabbed = false;
    cs->startingX = 0;
    cs->closestHouseIdx = -1;
    cs->closest2ndHouseIdx = -1;
    cs->closest3rdHouseIdx = -1;
}

inline bool HouseCarousel_Add(HouseCarouselState *cs, const char *houseName)
{
    if (!cs || cs->cardCount >= CAROUSEL_MAX_CARDS)
        return false;
    for (int i = 0; i < g_houseCatalogCount; i++)
    {
        if (strcmp(g_houseCatalog[i].name, houseName) == 0)
        {
            memcpy(&cs->items[cs->cardCount], &g_houseCatalog[i], sizeof(HouseCatalogItem));
            cs->cardCount++;
            return true;
        }
    }
    return false;
}

inline void HouseCarousel_SetupDefault(HouseCarouselState *cs)
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

    const char *defaults[] = {"Classic House", "Dry Fronts", "Long Oil", "Asym Split"};
    for (int i = 0; i < 4; i++)
    {
        HouseCarousel_Add(cs, defaults[i]);
    }
}
