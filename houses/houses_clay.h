#pragma once

#include "../clayton/clayton.h"
#include "houses.h"
#include "houses_carousel_clay.h"

inline void buildHousesWindowClay(Clayton *clayton, HouseCarouselState *houses, float deltaTime)
{
    if (!clayton || !clayton->shouldShowHouses)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    if (houses)
        HousesCarousel_Update(houses, deltaTime);

    CLAY(CLAY_ID("HousesContainer"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}},})
    {
        CLAY(CLAY_ID("HousesWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(
                CLAY_ID("HousesTitle"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 5, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT}}
            )
            {
                CLAY_TEXT(CLAY_STRING("Houses"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(CLAY_ID("HousesTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(clayton->housesCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("HousesBody"),
                {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM}}
            )
            {
                if (houses)
                {
                    HousesCarousel_Render(clayton, houses);

                    CLAY(CLAY_ID("HousesFooter"), CLAY_THEME_SHOP_CONTAINER_PADDING)
                    {
                        const int idx = houses->closestHouseIdx;
                        if (idx >= 0 && idx < houses->cardCount)
                        {
                            const HouseCatalogItem *item = &houses->items[idx];
                            CLAY_TEXT(ClayArena_FormatString(&clayton->clayArena, "Selected: %s", item->name),
                                      CLAY_TEXT_CONFIG(bodyCfg));
                        }
                        else
                        {
                            CLAY_TEXT(CLAY_STRING("Select a house"), CLAY_TEXT_CONFIG(bodyCfg));
                        }

                        CLAY(clayton->housesSelectClick.clayId, CLAY_THEME_BTN_BUY)
                        {
                            CLAY_TEXT(CLAY_STRING("SWITCH HOUSE"), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
                else
                {
                    CLAY_TEXT(CLAY_STRING("No houses"), CLAY_TEXT_CONFIG(labelCfg));
                }
            }
        }
    }
}

