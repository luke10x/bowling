#pragma once

#include "../clayton/clayton.h"
#include "../clayton/claytheme.h"
#include "settings.h"

inline Clay_String Settings_PwaStatusText(Clayton *clayton, const GameSettings *settings)
{
    if (!clayton || !settings)
        return CLAY_STRING("");

    switch (settings->pwaUpdateStatus)
    {
    case GamePwaUpdateStatus_Checking:
        return clayton->txl(TXL_PWA_UPDATE_CHECKING);
    case GamePwaUpdateStatus_UpToDate:
        return ClayArena_FormatString(
            &clayton->clayArena,
            Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_UP_TO_DATE_FMT),
            settings->latestBuildVersion[0] ? settings->latestBuildVersion : settings->currentBuildVersion
        );
    case GamePwaUpdateStatus_Available:
        return ClayArena_FormatString(
            &clayton->clayArena,
            Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_AVAILABLE_FMT),
            settings->currentBuildVersion,
            settings->latestBuildVersion[0] ? settings->latestBuildVersion : "?"
        );
    case GamePwaUpdateStatus_Offline:
        return clayton->txl(TXL_PWA_UPDATE_OFFLINE);
    case GamePwaUpdateStatus_Error:
        return clayton->txl(TXL_PWA_UPDATE_ERROR);
    case GamePwaUpdateStatus_Unsupported:
        return clayton->txl(TXL_PWA_UPDATE_UNSUPPORTED);
    case GamePwaUpdateStatus_Idle:
    case GamePwaUpdateStatus_Hidden:
    default:
        return ClayArena_FormatString(
            &clayton->clayArena,
            Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_INSTALLED_FMT),
            settings->currentBuildVersion
        );
    }
}

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

                CLAY(
                    CLAY_ID("SettingsActionsRow"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 10,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            },
                    }
                )
                {
                    CLAY(clayton->settingsResetProgressClick.clayId, CLAY_THEME_BTN_DANGER)
                    {
                        CLAY_TEXT(clayton->txl(TXL_RESET_PROGRESS), CLAY_TEXT_CONFIG(buttonCfg));
                    }
                    if (settings->pwaUpdateVisible)
                    {
                        CLAY(clayton->settingsCheckUpdateClick.clayId, CLAY_THEME_BTN_BOX)
                        {
                            CLAY_TEXT(clayton->txl(TXL_CHECK_FOR_UPDATE), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                        if (settings->canApplyPwaUpdate())
                        {
                            CLAY(clayton->settingsApplyUpdateClick.clayId, CLAY_THEME_BTN_SUCCESS)
                            {
                                CLAY_TEXT(clayton->txl(TXL_UPDATE_PWA), CLAY_TEXT_CONFIG(buttonCfg));
                            }
                        }
                    }
                }
                CLAY_TEXT(
                    clayton->txl(TXL_RESET_PROGRESS_HELP),
                    CLAY_TEXT_CONFIG(bodyCfg)
                );
                if (settings->pwaUpdateVisible)
                {
                    CLAY_TEXT(
                        Settings_PwaStatusText(clayton, settings),
                        CLAY_TEXT_CONFIG(bodyCfg)
                    );
                }
            }
        }
    }
}
