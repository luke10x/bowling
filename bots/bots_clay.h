#pragma once

#include "../clayton/clayton.h"
#include "bots.h"
#include "bots_carousel_clay.h"

inline void buildBotsWindowClay(Clayton *clayton, BotCarouselState *bots, float deltaTime)
{
    if (!clayton || !clayton->shouldShowBotSelect)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    if (bots)
        BotsCarousel_Update(bots, deltaTime);

    CLAY(
        CLAY_ID("BotsContainer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {0, 0, 0, 0},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
        }
    )
    {
        CLAY(CLAY_ID("BotsWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(
                CLAY_ID("BotsTitle"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 5, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY_TEXT(CLAY_STRING("Bot Avatar"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(CLAY_ID("BotsTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(clayton->botSelectCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("BotsBody"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM}}
            )
            {
                if (bots)
                {
                    BotsCarousel_Render(clayton, bots);

                    CLAY(CLAY_ID("BotsFooter"), CLAY_THEME_SHOP_CONTAINER_PADDING)
                    {
                        const int idx = bots->closestBotIdx;
                        if (idx >= 0 && idx < bots->cardCount)
                        {
                            const BotCatalogItem *item = &bots->items[idx];
                            CLAY_TEXT(ClayArena_FormatString(&clayton->clayArena, "Selected: %s", item->name),
                                      CLAY_TEXT_CONFIG(bodyCfg));
                        }
                        else
                        {
                            CLAY_TEXT(CLAY_STRING("Select a bot avatar"), CLAY_TEXT_CONFIG(bodyCfg));
                        }

                        CLAY(clayton->botSelectSelectClick.clayId, CLAY_THEME_BTN_BUY)
                        {
                            CLAY_TEXT(CLAY_STRING("SELECT BOT"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
                else
                {
                    CLAY_TEXT(CLAY_STRING("No bots"), CLAY_TEXT_CONFIG(labelCfg));
                }
            }
        }
    }
}

