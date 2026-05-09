#pragma once

#include "../clayton/clayton.h"
#include "houses.h"
#include <cfloat>
#include <glm/glm.hpp>

// Mirrors the shop carousel behavior, but uses distinct Clay IDs to avoid collisions.

inline float HousesCarousel_GetCenteredOffset(const HouseCarouselState *cs)
{
    float offset = cs->scrollOffset;
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("HousesCarouselBelt"));
    float cardWidth = (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
    float baseOffset = ((float)cd.boundingBox.width - cardWidth) / 2.0f;
    return baseOffset + offset;
}

inline float HousesCarousel_GetSlotWidth(const HouseCarouselState *cs)
{
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("HousesCarouselBelt"));
    return (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
}

inline void HousesCarousel_UpdateClosestIndices(HouseCarouselState *cs, float slotWidth)
{
    if (cs->cardCount <= 0 || slotWidth <= 0.0f)
    {
        cs->closestHouseIdx = -1;
        cs->closest2ndHouseIdx = -1;
        return;
    }

    float exact = cs->scrollOffset / slotWidth;
    int nearestSlot = (int)glm::round(exact);
    nearestSlot = glm::clamp(nearestSlot, 1 - cs->cardCount, 0);
    int closestIdx = -nearestSlot;
    cs->closestHouseIdx = closestIdx;

    cs->closest2ndHouseIdx = -1;
    if (cs->cardCount > 1)
    {
        int candidates[2] = {closestIdx - 1, closestIdx + 1};
        float bestDist = FLT_MAX;
        for (int cand : candidates)
        {
            if (cand < 0 || cand >= cs->cardCount)
                continue;
            float slotForCand = -(float)cand;
            float dist = glm::abs(exact - slotForCand);
            if (dist < bestDist)
            {
                bestDist = dist;
                cs->closest2ndHouseIdx = cand;
            }
        }
    }
}

inline void HousesCarousel_OnPointerDown(HouseCarouselState *cs, float x)
{
    if (!Clay_PointerOver(CLAY_ID("HousesCarouselContainer")))
        return;
    cs->isGrabbed = true;
    cs->startingX = (int)x;
}

inline void HousesCarousel_OnPointerMove(HouseCarouselState *cs, float dx)
{
    if (!cs->isGrabbed)
        return;

    float slotWidth = HousesCarousel_GetSlotWidth(cs);
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    nearest = glm::clamp(nearest, 0, 1 - cs->cardCount);
    float targetPos = (float)nearest * slotWidth;
    float error = targetPos - cs->scrollOffset;
    float dist = glm::abs(error);
    float normDist = glm::min(dist / (0.5f * slotWidth), 1.0f);

#if defined(__EMSCRIPTEN__)
    const float MIN_SCALE = 0.8f;
    const float MAX_SCALE = 1.6f;
#else
    const float MIN_SCALE = 0.3f;
    const float MAX_SCALE = 0.6f;
#endif
    float scale = MIN_SCALE + (MAX_SCALE - MIN_SCALE) * (normDist * normDist);
    cs->scrollOffset += dx * scale;
    HousesCarousel_UpdateClosestIndices(cs, slotWidth);
}

inline void HousesCarousel_OnPointerUp(HouseCarouselState *cs)
{
    cs->isGrabbed = false;
    cs->velocity = 0.0f;
}

inline void HousesCarousel_Update(HouseCarouselState *cs, float deltaTime)
{
    if (cs->isGrabbed)
        return;

    float slotWidth = HousesCarousel_GetSlotWidth(cs);
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    nearest = glm::clamp(nearest, 1 - cs->cardCount, 0);
    float targetPos = (float)nearest * slotWidth;
    float error = targetPos - cs->scrollOffset;

    const float Kp = 8.0f;
    float targetVelocity = error * Kp;
    const float approachRate = 12.0f;
    float blend = 1.0f - glm::exp(-approachRate * deltaTime);
    cs->velocity += (targetVelocity - cs->velocity) * blend;
    cs->scrollOffset += cs->velocity * deltaTime;

    if (glm::abs(error) < 0.1f && glm::abs(cs->velocity) < 0.5f)
    {
        cs->scrollOffset = targetPos;
        cs->velocity = 0.0f;
    }

    HousesCarousel_UpdateClosestIndices(cs, slotWidth);
}

inline void DrawHouseCard(Clayton *clayton, const HouseCarouselState *carousel, const HouseCatalogItem *item, int idx)
{
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig rarityCfg = CLAY_THEME_TEXT_RARITY;
    Clay_ElementDeclaration rarityBadgeDecl = CLAY_THEME_RARITY_BADGE;
    Clay_LayoutConfig rarityBadgeLayoutCfg = rarityBadgeDecl.layout;
    ClayArena *arena = &clayton->clayArena;

    CLAY(
        CLAY_IDI("HousesCarouselCard", idx),
        {.layout = {.sizing = {CLAY_SIZING_PERCENT(CAROUSEL_CARD_WIDTH), CLAY_SIZING_GROW()}}}
    )
    {
        CLAY(CLAY_IDI("HousesCatalogItemWrapper", idx), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, .padding = {12, 12, 12, 12}}})
        {
            CLAY(CLAY_IDI("HousesCatalogItem", idx), CLAY_THEME_CATALOG_ITEM)
            {
                CLAY(
                    CLAY_IDI("HousesItemHeader", idx),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .backgroundColor = {180, 180, 220, (float)(Clay_Hovered() ? 120 : 180)}}
                )
                {
                    Clay_String nameStr = ClayArena_FormatString(arena, "%s", item->name);
                    CLAY_TEXT(nameStr, CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY(CLAY_IDI("HousesTitleSpacer", idx), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}

                    Clay_Color rarityColor = CLAY_COLOR_RARITY_COMMON;
                    if (strcmp(item->rarity, "LEGENDARY") == 0)
                        rarityColor = CLAY_COLOR_RARITY_LEGENDARY;
                    if (strcmp(item->rarity, "EPIC") == 0)
                        rarityColor = CLAY_COLOR_RARITY_EPIC;
                    if (strcmp(item->rarity, "RARE") == 0)
                        rarityColor = CLAY_COLOR_RARITY_RARE;
                    CLAY(CLAY_IDI("HousesRarityBadge", idx), {.layout = rarityBadgeLayoutCfg, .backgroundColor = rarityColor})
                    {
                        Clay_String rarityStr = ClayArena_FormatString(arena, "%s", item->rarity);
                        CLAY_TEXT(rarityStr, CLAY_TEXT_CONFIG(rarityCfg));
                    }
                }

                // Preview (reuse texture slots 1/2, like shop).
                CLAY(CLAY_IDI("HousesPreview", idx), CLAY_THEME_BALL_PREVIEW)
                {
                    if (idx == carousel->closestHouseIdx)
                    {
                        CLAY(CLAY_IDI("HousesIconImage1-", idx), {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(120)}}, .image = {.imageData = &clayton->housesPinImage}}) {}
                    }
                    else if (idx == carousel->closest2ndHouseIdx)
                    {
                        CLAY(CLAY_IDI("HousesIconImage2-", idx), {.layout = {.sizing = {.width = CLAY_SIZING_FIXED(100), .height = CLAY_SIZING_FIXED(120)}}, .image = {.imageData = &clayton->housesPin2Image}}) {}
                    }
                }
            }
        }
    }
}

inline void HousesCarousel_Render(Clayton *clayton, const HouseCarouselState *carousel)
{
    float offset = HousesCarousel_GetCenteredOffset(carousel);
    CLAY(
        CLAY_ID("HousesCarouselContainer"),
        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(420)}, .padding = {10, 10, 10, 10}},
         .backgroundColor = {0, 0, 0, 100},
         .clip = {.horizontal = true, .vertical = false, .childOffset = {offset, 0}}}
    )
    {
        CLAY(
            CLAY_ID("HousesCarouselBelt"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childGap = (int)CAROUSEL_CARD_SPACING,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            for (int i = 0; i < carousel->cardCount; i++)
            {
                DrawHouseCard(clayton, carousel, &carousel->items[i], i);
            }
        }
    }
}
