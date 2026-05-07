#pragma once

#include "../clayton/clayton.h"

inline void buildOilStatusWindowClay(Clayton *clayton)
{
    if (!clayton || !clayton->shouldShowOilStatus)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;

    CLAY(
        CLAY_ID("OilStatusContainer"),
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
        CLAY(CLAY_ID("OilStatusWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(
                CLAY_ID("OilStatusTitle"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 5, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                }
            )
            {
                CLAY_TEXT(CLAY_STRING("🛢 Oil Status"), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(
                    CLAY_ID("OilStatusTitleDivider"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}
                ){};
                CLAY(clayton->oilStatusCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            // Intentionally empty for now (placeholder window).
            CLAY(
                CLAY_ID("OilStatusBody"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(220)},
                            .padding = {10, 10, 10, 10},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                }
            )
            {
                CLAY(
                    CLAY_ID("OilStatusPreviewImage"),
                    {
                        .layout =
                            {.sizing = {.width = CLAY_SIZING_FIXED(220), .height = CLAY_SIZING_FIXED(220)}},
                        .image = {.imageData = &clayton->oilImage},
                    }
                )
                {
                }
            }
        }
    }
}
