#pragma once

#include "../clayton/clayton.h"
#include "../clayton/claytheme.h"
#include "settings.h"

inline Clay_String SettingsWebUpdateStatusText(Clayton *clayton, const GameSettings *settings)
{
    if (!clayton || !settings)
        return CLAY_STRING("");

    switch (settings->webUpdateStatus)
    {
        case GameSettings::WEB_UPDATE_CHECKING:
            return clayton->txl(TXL_PWA_UPDATE_CHECKING);
        case GameSettings::WEB_UPDATE_UP_TO_DATE:
            return ClayArena_FormatString(
                &clayton->clayArena,
                Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_UP_TO_DATE_FMT),
                settings->publishedBuild[0] ? settings->publishedBuild : settings->installedBuild
            );
        case GameSettings::WEB_UPDATE_AVAILABLE:
            return ClayArena_FormatString(
                &clayton->clayArena,
                Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_AVAILABLE_FMT),
                settings->installedBuild[0] ? settings->installedBuild : "?",
                settings->publishedBuild[0] ? settings->publishedBuild : "?"
            );
        case GameSettings::WEB_UPDATE_OFFLINE:
            return clayton->txl(TXL_PWA_UPDATE_OFFLINE);
        case GameSettings::WEB_UPDATE_ERROR:
            return clayton->txl(TXL_PWA_UPDATE_ERROR);
        case GameSettings::WEB_UPDATE_APPLYING:
            return clayton->txl(TXL_PWA_UPDATE_APPLYING);
        case GameSettings::WEB_UPDATE_IDLE:
            return ClayArena_FormatString(
                &clayton->clayArena,
                Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_INSTALLED_FMT),
                settings->installedBuild[0] ? settings->installedBuild : "?"
            );
        case GameSettings::WEB_UPDATE_UNSUPPORTED:
        default:
            return clayton->txl(TXL_PWA_UPDATE_UNSUPPORTED);
    }
}

inline Clay_String SettingsWebUpdateInstalledText(Clayton *clayton, const GameSettings *settings)
{
    if (!clayton || !settings)
        return CLAY_STRING("");
    return ClayArena_FormatString(
        &clayton->clayArena,
        Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_INSTALLED_FMT),
        settings->installedBuild[0] ? settings->installedBuild : "?"
    );
}

inline Clay_String SettingsWebUpdatePublishedText(Clayton *clayton, const GameSettings *settings)
{
    if (!clayton || !settings)
        return CLAY_STRING("");
    return ClayArena_FormatString(
        &clayton->clayArena,
        Txl_Get(clayton->uiLanguage, TXL_PWA_UPDATE_PUBLISHED_FMT),
        settings->publishedBuild[0] ? settings->publishedBuild : "?"
    );
}

inline Clay_String SettingsWebUpdateActionLabel(Clayton *clayton, const GameSettings *settings)
{
    if (!clayton || !settings)
        return CLAY_STRING("");
    return settings->webUpdateStandalone
        ? clayton->txl(TXL_UPDATE_PWA)
        : clayton->txl(TXL_UPDATE_WEB);
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

#ifdef __EMSCRIPTEN__
                CLAY(
                    CLAY_ID("SettingsUpdateSection"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                    }
                )
                {
                    CLAY_TEXT(SettingsWebUpdateInstalledText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY_TEXT(SettingsWebUpdatePublishedText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY_TEXT(SettingsWebUpdateStatusText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    if (settings->webUpdateStatus == GameSettings::WEB_UPDATE_CHECKING ||
                        settings->webUpdateStatus == GameSettings::WEB_UPDATE_APPLYING)
                    {
                    }
                    else if (settings->webUpdateAvailable())
                    {
                        CLAY(clayton->settingsApplyUpdateClick.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(SettingsWebUpdateActionLabel(clayton, settings), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                    else
                    {
                        CLAY(clayton->settingsCheckUpdateClick.clayId, CLAY_THEME_BTN_PRIMARY)
                        {
                            CLAY_TEXT(clayton->txl(TXL_CHECK_FOR_UPDATE), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
#endif

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
