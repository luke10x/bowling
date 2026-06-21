#pragma once

#include "../clayton/clayton.h"
#include "../clayton/claytheme.h"
#include "settings.h"

inline void buildSettingsWindowClay(Clayton *clayton, GameSettings *settings)
{
    if (!clayton || !clayton->shouldShowSettings || !settings)
        return;

    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    CLAY(
        CLAY_ID("SettingsContainer"),
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
        CLAY(CLAY_ID("SettingsWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(
                CLAY_ID("SettingsTitleRow"),
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
                CLAY_TEXT(clayton->txl(TXL_GAME_SETTINGS_TITLE), CLAY_TEXT_CONFIG(titleCfg));
                CLAY(CLAY_ID("SettingsTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(clayton->settingsCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonCfg));
                }
            }

            CLAY(
                CLAY_ID("SettingsBody"),
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
                CLAY_TEXT(
                    ClayArena_FormatString(
                        &clayton->clayArena,
                        Txl_Get(clayton->uiLanguage, TXL_SNOWFLAKES_FMT),
                        settings->snowflakeCount,
                        settings->maxSnowflakes
                    ),
                    CLAY_TEXT_CONFIG(bodyCfg)
                );
                ClaytonSlider_Render(&settings->snowflakeSlider, clayton, Txl_Get(clayton->uiLanguage, TXL_SNOWFLAKE_DENSITY), "");
                if (settings->snowflakeCount == 0)
                {
                    CLAY_TEXT(
                        clayton->txl(TXL_SNOW_DISABLED),
                        CLAY_TEXT_CONFIG(bodyCfg)
                    );
                }

                CLAY(clayton->settingsResetProgressClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(clayton->txl(TXL_RESET_PROGRESS), CLAY_TEXT_CONFIG(buttonCfg));
                }
                CLAY_TEXT(
                    clayton->txl(TXL_RESET_PROGRESS_HELP),
                    CLAY_TEXT_CONFIG(bodyCfg)
                );
            }
        }
    }
}
