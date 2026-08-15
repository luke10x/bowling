#pragma once

#include "../clayton/clayton.h"
#include "bots.h"
#include <cfloat>
#include <glm/glm.hpp>

// Mirrors the houses carousel behavior, but uses distinct Clay IDs to avoid collisions.

inline float BotsCarousel_GetCenteredOffset(const BotCarouselState *cs)
{
    float offset = cs->scrollOffset;
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("BotsCarouselBelt"));
    float cardWidth = (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
    float baseOffset = ((float)cd.boundingBox.width - cardWidth) / 2.0f;
    return baseOffset + offset;
}

inline float BotsCarousel_GetSlotWidth(const BotCarouselState *cs)
{
    (void)cs;
    Clay_ElementData cd = Clay_GetElementData(CLAY_ID("BotsCarouselBelt"));
    return (float)cd.boundingBox.width * CAROUSEL_CARD_WIDTH;
}

inline void BotsCarousel_UpdateClosestIndices(BotCarouselState *cs, float slotWidth)
{
    if (cs->cardCount <= 0 || slotWidth <= 0.0f)
    {
        cs->closestBotIdx = -1;
        cs->closest2ndBotIdx = -1;
        cs->closest3rdBotIdx = -1;
        return;
    }

    // Match Shop/Houses convention:
    // - slot 0 is centered at scrollOffset=0
    // - moving to the right (next cards) drives scrollOffset negative, slotForI=-i
    float exact = cs->scrollOffset / slotWidth;
    struct Candidate
    {
        int idx;
        float dist;
    };

    Candidate best[3] = {
        {-1, FLT_MAX},
        {-1, FLT_MAX},
        {-1, FLT_MAX},
    };

    for (int i = 0; i < cs->cardCount; ++i)
    {
        float slotForI = -(float)i;
        float d = glm::abs(exact - slotForI);
        Candidate c = {i, d};
        for (int k = 0; k < 3; ++k)
        {
            if (c.dist < best[k].dist)
            {
                for (int j = 2; j > k; --j)
                    best[j] = best[j - 1];
                best[k] = c;
                break;
            }
        }
    }

    cs->closestBotIdx = best[0].idx;
    cs->closest2ndBotIdx = best[1].idx;
    cs->closest3rdBotIdx = best[2].idx;
}

inline void BotsCarousel_OnPointerDown(BotCarouselState *cs, float x)
{
    if (!cs)
        return;
    if (!Clay_PointerOver(CLAY_ID("BotsCarouselContainer")))
        return;
    cs->isGrabbed = true;
    cs->startingX = (int)x;
    cs->isDragging = true;
}

inline void BotsCarousel_OnPointerMove(BotCarouselState *cs, float dx)
{
    if (!cs || !cs->isGrabbed)
        return;

    float slotWidth = BotsCarousel_GetSlotWidth(cs);
    int nearest = (int)glm::round(cs->scrollOffset / slotWidth);
    nearest = glm::clamp(nearest, 1 - cs->cardCount, 0);
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
    cs->scrollOffset += dx * scale * 1.5f;
    BotsCarousel_UpdateClosestIndices(cs, slotWidth);
}

inline void BotsCarousel_OnPointerUp(BotCarouselState *cs)
{
    if (!cs)
        return;
    cs->isGrabbed = false;
    cs->isDragging = false;
    cs->velocity = 0.0f;
}

inline void BotsCarousel_Update(BotCarouselState *cs, float deltaTime)
{
    if (!cs)
        return;
    if (cs->isGrabbed)
        return;

    float slotWidth = BotsCarousel_GetSlotWidth(cs);
    if (cs->cardCount <= 0 || slotWidth <= 0.0f)
        return;

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

    BotsCarousel_UpdateClosestIndices(cs, slotWidth);
}

inline void DrawBotCard(Clayton *clayton, const BotCarouselState *carousel, const BotCatalogItem *item, int idx)
{
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig rarityCfg = CLAY_THEME_TEXT_RARITY;
    Clay_ElementDeclaration rarityBadgeDecl = CLAY_THEME_RARITY_BADGE;
    Clay_LayoutConfig rarityBadgeLayoutCfg = rarityBadgeDecl.layout;
    ClayArena *arena = &clayton->clayArena;
    Clay_ElementData beltCd = Clay_GetElementData(CLAY_ID("BotsCarouselBelt"));
    float slotWidthPx = (float)beltCd.boundingBox.width * CAROUSEL_CARD_WIDTH;
    float previewSizePx = glm::max(80.0f, slotWidthPx - 48.0f);

    CLAY(
        CLAY_IDI("BotsCarouselCard", idx),
        {.layout = {.sizing = {CLAY_SIZING_PERCENT(CAROUSEL_CARD_WIDTH), CLAY_SIZING_GROW()}}}
    )
    {
        CLAY(CLAY_IDI("BotsCatalogItemWrapper", idx), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, .padding = {12, 12, 12, 12}}})
        {
            CLAY(CLAY_IDI("BotsCatalogItem", idx), CLAY_THEME_CATALOG_ITEM)
            {
                CLAY(
                    CLAY_IDI("BotsItemHeader", idx),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT},
                     .backgroundColor = {180, 180, 220, (float)(Clay_Hovered() ? 120 : 180)}}
                )
                {
                    Clay_String nameStr = ClayArena_FormatString(arena, "%s", item->name);
                    CLAY_TEXT(nameStr, CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY(CLAY_IDI("BotsTitleSpacer", idx), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}

                    Clay_Color rarityColor = CLAY_COLOR_RARITY_COMMON;
                    if (item->rarity && strcmp(item->rarity, "LEGENDARY") == 0)
                        rarityColor = CLAY_COLOR_RARITY_LEGENDARY;
                    if (item->rarity && strcmp(item->rarity, "EPIC") == 0)
                        rarityColor = CLAY_COLOR_RARITY_EPIC;
                    if (item->rarity && strcmp(item->rarity, "RARE") == 0)
                        rarityColor = CLAY_COLOR_RARITY_RARE;
                    CLAY(CLAY_IDI("BotsRarityBadge", idx), {.layout = rarityBadgeLayoutCfg, .backgroundColor = rarityColor})
                    {
                        Clay_String rarityStr = ClayArena_FormatString(arena, "%s", item->rarity ? item->rarity : "COMMON");
                        CLAY_TEXT(rarityStr, CLAY_TEXT_CONFIG(rarityCfg));
                    }
                }

                CLAY(
                    CLAY_IDI("BotsPreview", idx),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(previewSizePx)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                        .cornerRadius = {CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD, CLAY_RADIUS_MD},
                    }
                )
                {
                    // Use the same 3 preview slots as other carousels:
                    // - closest: texture slot 1
                    // - 2nd: slot 2
                    // - 3rd: slot 3
                    if (idx == carousel->closestBotIdx)
                    {
                        CLAY(
                            CLAY_IDI("BotsPreviewImage1", idx),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->botPreviewImage}}
                        )
                        {
                        }
                    }
                    else if (idx == carousel->closest2ndBotIdx)
                    {
                        CLAY(
                            CLAY_IDI("BotsPreviewImage2", idx),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->botPreview2Image}}
                        )
                        {
                        }
                    }
                    else if (idx == carousel->closest3rdBotIdx)
                    {
                        CLAY(
                            CLAY_IDI("BotsPreviewImage3", idx),
                            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},
                             .image = {.imageData = &clayton->botPreview3Image}}
                        )
                        {
                        }
                    }
                }
            }
        }
    }
}

inline void BotsCarousel_Render(Clayton *clayton, const BotCarouselState *carousel)
{
    float offset = BotsCarousel_GetCenteredOffset(carousel);
    CLAY(
        CLAY_ID("BotsCarouselContainer"),
        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(420)}, .padding = {10, 10, 10, 10}},
         .backgroundColor = {0, 0, 0, 100},
         .clip = {.horizontal = true, .vertical = false, .childOffset = {offset, 0}}}
    )
    {
        CLAY(
            CLAY_ID("BotsCarouselBelt"),
            {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childGap = (int)CAROUSEL_CARD_SPACING,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT}}
        )
        {
            for (int i = 0; i < carousel->cardCount; i++)
            {
                DrawBotCard(clayton, carousel, &carousel->items[i], i);
            }
        }
    }
}
