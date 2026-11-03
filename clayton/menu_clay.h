#pragma once

#include "clayton.h"
#include "claytheme.h"
#include "language_flag_clay.h"

inline void MenuIconButtonContents(Clayton *clayton, int slot, Gles3_ImageConfig *icon,
                                   TxlKey label, Clay_TextElementConfig textConfig)
{
    // Match the flag button: equal side cells keep every translated label centered.
    constexpr float kSideCellWidth = 46.0f;
    constexpr float kIconSize = 34.0f;
    CLAY(CLAY_IDI("MenuIconLeft", slot),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(kSideCellWidth), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
    {
        CLAY(CLAY_IDI("MenuIconImage", slot),
             {.layout = {.sizing = {CLAY_SIZING_FIXED(kIconSize), CLAY_SIZING_FIXED(kIconSize)}},
              .image = {.imageData = icon},
              .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM},
              .border = {.color = {160, 170, 200, 160}, .width = CLAY_BORDER_ALL(1)}}) {}
    }
    CLAY(CLAY_IDI("MenuIconLabel", slot),
         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
    {
        CLAY_TEXT(clayton->txl(label), CLAY_TEXT_CONFIG(textConfig));
    }
    CLAY(CLAY_IDI("MenuIconRight", slot),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(kSideCellWidth), CLAY_SIZING_GROW()}}}) {}
}

inline void buildMenuWindowClay(Clayton *clayton, bool showGoToSchool, bool showTracker)
{
    if (!clayton)
        return;

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    ClayArena *arena = &clayton->clayArena;

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
                CLAY_TEXT(clayton->txl(TXL_MENU), CLAY_TEXT_CONFIG(titleFontCfg));
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
                Clay_ElementDeclaration iconButton = LanguageFlagButtonStyle();
                CLAY(clayton->menuCampaignClick.clayId, iconButton)
                {
                    MenuIconButtonContents(clayton, 0, &clayton->menuCampaignImage, TXL_CAMPAIGN, buttonFontCfg);
                }

                if (showGoToSchool)
                {
                    CLAY(clayton->menuSchoolClick.clayId, iconButton)
                    {
                        MenuIconButtonContents(clayton, 1, &clayton->menuSchoolImage, TXL_GO_TO_SCHOOL, buttonFontCfg);
                    }
                }

                Clay_ElementDeclaration languageButton = LanguageFlagButtonStyle();
                CLAY(clayton->menuLanguageClick.clayId, languageButton)
                {
                    LanguageFlagButtonContents(clayton, 4, clayton->uiLanguage, TXL_LANGUAGE, buttonFontCfg);
                }

                // CLAY(clayton->menuPracticeClick.clayId, CLAY_THEME_BTN_HUD)
                // {
                //     CLAY_TEXT(clayton->txl(TXL_PRACTICE), CLAY_TEXT_CONFIG(buttonFontCfg));
                // }

                if (clayton->minigamesMenuUnlocked)
                {
                    CLAY(clayton->menuMinigamesClick.clayId, CLAY_THEME_BTN_HUD)
                    {
                        CLAY_TEXT(clayton->txl(TXL_MINIGAMES), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }

                // CLAY(clayton->menuDeviceShareClick.clayId, CLAY_THEME_BTN_HUD)
                // {
                //     CLAY_TEXT(clayton->txl(TXL_DEVICE_SHARE), CLAY_TEXT_CONFIG(buttonFontCfg));
                // }

                CLAY(clayton->menuSettingsClick.clayId, iconButton)
                {
                    MenuIconButtonContents(clayton, 2, &clayton->menuSettingsImage, TXL_GAME_SETTINGS, buttonFontCfg);
                }

                if (showTracker)
                {
                    CLAY(clayton->menuTrackerClick.clayId, iconButton)
                    {
                        MenuIconButtonContents(clayton, 4, &clayton->menuTrackerImage, TXL_TRACKER, buttonFontCfg);
                    }
                }

                CLAY(clayton->menuCreditsClick.clayId, iconButton)
                {
                    MenuIconButtonContents(clayton, 3, &clayton->menuCreditsImage, TXL_CREDITS, buttonFontCfg);
                }

                if (clayton->secretMenuUnlock.unlocked)
                {
                    CLAY(clayton->menuRenameClick.clayId, CLAY_THEME_BTN_HUD)
                    {
                        CLAY_TEXT(clayton->txl(TXL_RENAME), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }
            }
        }
    }
}
