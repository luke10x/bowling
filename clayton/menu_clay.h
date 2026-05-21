#pragma once

#include "clayton.h"
#include "claytheme.h"

inline void buildMenuWindowClay(Clayton *clayton, bool showGoToSchool)
{
    if (!clayton)
        return;

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;

    // Container for hit-testing in win_stack.
    CLAY(
        CLAY_ID("MenuContainer"),
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
        CLAY(CLAY_ID("MenuWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            // Title row with close button
            CLAY(
                CLAY_ID("MenuTitleRow"),
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
                CLAY_TEXT(CLAY_STRING("Menu"), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY(CLAY_ID("MenuTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(clayton->menuCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }

            // Buttons
            CLAY(
                CLAY_ID("MenuButtons"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                }
            )
            {
                CLAY(clayton->menuRenameClick.clayId, CLAY_THEME_BTN_HUD)
                {
                    CLAY_TEXT(CLAY_STRING("Rename"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }

                CLAY(clayton->menuBotSelectClick.clayId, CLAY_THEME_BTN_HUD)
                {
                    CLAY_TEXT(CLAY_STRING("Bot avatar"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }

                if (showGoToSchool)
                {
                    CLAY(clayton->menuSchoolClick.clayId, CLAY_THEME_BTN_HUD)
                    {
                        CLAY_TEXT(CLAY_STRING("Go to school"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }
            }
        }
    }
}
