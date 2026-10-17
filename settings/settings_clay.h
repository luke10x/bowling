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

#ifdef __EMSCRIPTEN__
inline Clay_Color SettingsWebUpdateBusyColor()
{
    // Pulse the stationary button instead of inserting a spinner that changes layout.
    const float phase = (SDL_GetTicks() % 1200) / 600.0f;
    const float lift = 36.0f * (phase <= 1.0f ? phase : 2.0f - phase);
    Clay_Color color = CLAY_COLOR_BTN_PRIMARY;
    color.r += lift;
    color.g += lift;
    color.b += lift * 0.4f;
    return color;
}
#endif

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
                    const bool checking = settings->webUpdateStatus == GameSettings::WEB_UPDATE_CHECKING;
                    const bool applying = settings->webUpdateStatus == GameSettings::WEB_UPDATE_APPLYING;
                    CLAY(CLAY_ID("SettingsUpdateInstalledSlot"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(24)}}})
                    {
                        CLAY_TEXT(SettingsWebUpdateInstalledText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    }
                    CLAY(CLAY_ID("SettingsUpdatePublishedSlot"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(24)}}})
                    {
                        CLAY_TEXT(SettingsWebUpdatePublishedText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    }
                    CLAY(CLAY_ID("SettingsUpdateStatusSlot"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(64)}}})
                    {
                        if (!checking && !applying && settings->webUpdateStatus != GameSettings::WEB_UPDATE_IDLE)
                            CLAY_TEXT(SettingsWebUpdateStatusText(clayton, settings), CLAY_TEXT_CONFIG(bodyCfg));
                    }
                    Clay_ElementDeclaration updateButton = CLAY_THEME_BTN_PRIMARY;
                    updateButton.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)};
                    if (checking || applying)
                    {
                        updateButton.backgroundColor = SettingsWebUpdateBusyColor();
                        CLAY(CLAY_ID("SettingsUpdateBusyButton"), updateButton)
                        {
                            CLAY_TEXT(clayton->txl(checking ? TXL_PWA_UPDATE_CHECKING : TXL_PWA_UPDATE_APPLYING),
                                      CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                    else if (settings->webUpdateAvailable())
                    {
                        updateButton.backgroundColor = CLAY_COLOR_BTN_SUCCESS;
                        CLAY(clayton->settingsApplyUpdateClick.clayId, updateButton)
                        {
                            CLAY_TEXT(SettingsWebUpdateActionLabel(clayton, settings), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                    else
                    {
                        CLAY(clayton->settingsCheckUpdateClick.clayId, updateButton)
                        {
                            CLAY_TEXT(clayton->txl(TXL_CHECK_FOR_UPDATE), CLAY_TEXT_CONFIG(buttonCfg));
                        }
                    }
                }
#endif

                CLAY(
                    CLAY_ID("SettingsHighDetailsRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childGap = 12,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    CLAY_TEXT(clayton->txl(TXL_HIGH_DETAILS), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY(CLAY_ID("SettingsHighDetailsSpacer"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                    Clay_ElementDeclaration checkbox = CLAY_THEME_BTN_BOX;
                    checkbox.backgroundColor = settings->highDetailsEnabled
                        ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                    CLAY(clayton->settingsHighDetailsClick.clayId, checkbox)
                    {
                        CLAY_TEXT(settings->highDetailsEnabled
                                      ? CLAY_STRING("✓") : CLAY_STRING(" "),
                                  CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                CLAY(
                    CLAY_ID("SettingsPatchedYmfmRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childGap = 12,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    CLAY_TEXT(clayton->txl(TXL_PATCHED_YMFM), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY(CLAY_ID("SettingsPatchedYmfmSpacer"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                    Clay_ElementDeclaration checkbox = CLAY_THEME_BTN_BOX;
                    checkbox.backgroundColor = settings->patchedYmfmEnabled
                        ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                    CLAY(clayton->settingsPatchedYmfmClick.clayId, checkbox)
                    {
                        CLAY_TEXT(settings->patchedYmfmEnabled
                                      ? CLAY_STRING("✓") : CLAY_STRING(" "),
                                  CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                CLAY(
                    CLAY_ID("SettingsPerformanceAnalysisRow"),
                    {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childGap = 12,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT}}
                )
                {
                    CLAY_TEXT(clayton->txl(TXL_PERFORMANCE_ANALYSIS), CLAY_TEXT_CONFIG(bodyCfg));
                    CLAY(CLAY_ID("SettingsPerformanceAnalysisSpacer"),
                         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                    Clay_ElementDeclaration checkbox = CLAY_THEME_BTN_BOX;
                    checkbox.backgroundColor = settings->performanceAnalysisEnabled
                        ? CLAY_COLOR_BTN_SUCCESS : CLAY_COLOR_BTN_DISABLED;
                    CLAY(clayton->settingsPerformanceAnalysisClick.clayId, checkbox)
                    {
                        CLAY_TEXT(settings->performanceAnalysisEnabled
                                      ? CLAY_STRING("✓") : CLAY_STRING(" "),
                                  CLAY_TEXT_CONFIG(buttonCfg));
                    }
                }

                Clay_ElementDeclaration resetProgressButton = CLAY_THEME_BTN_DANGER;
                resetProgressButton.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(56)};
                resetProgressButton.layout.padding = {.left = 14, .right = 14, .top = 0, .bottom = 0};
                Clay_TextElementConfig resetButtonCfg = buttonCfg;
                resetButtonCfg.wrapMode = CLAY_TEXT_WRAP_NONE;
                Clay_TextElementConfig resetHelpCfg = bodyCfg;
                resetHelpCfg.wrapMode = CLAY_TEXT_WRAP_WORDS;

                CLAY(
                    CLAY_ID("SettingsResetProgressRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 12,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    }
                )
                {
                    CLAY(
                        CLAY_ID("SettingsResetProgressHelpWrap"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            },
                        }
                    )
                    {
                        CLAY_TEXT(
                            clayton->txl(TXL_RESET_PROGRESS_HELP),
                            CLAY_TEXT_CONFIG(resetHelpCfg)
                        );
                    }
                    CLAY(
                        CLAY_ID("SettingsResetProgressButtons"),
                        {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}, .childGap = 10,
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM}}
                    )
                    {
                        CLAY(clayton->settingsResetCampaignClick.clayId, resetProgressButton)
                        {
                            CLAY_TEXT(clayton->txl(TXL_RESET_CAMPAIGN), CLAY_TEXT_CONFIG(resetButtonCfg));
                        }
                        CLAY(clayton->settingsResetProgressClick.clayId, resetProgressButton)
                        {
                            CLAY_TEXT(clayton->txl(TXL_RESET_PROGRESS), CLAY_TEXT_CONFIG(resetButtonCfg));
                        }
                    }
                }
            }
        }
    }
}
