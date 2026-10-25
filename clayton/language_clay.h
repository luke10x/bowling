#pragma once

#include "clayton.h"
#include "claytheme.h"
#include "language_flag_clay.h"

inline void buildLanguageWindowClay(Clayton *clayton)
{
    if (!clayton)
        return;

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;

    CLAY(
        CLAY_ID("LanguageContainer"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    )
    {
        CLAY(CLAY_ID("LanguageWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(
                CLAY_ID("LanguageTitleRow"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {0, 0, 5, 0},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }
            )
            {
                CLAY_TEXT(clayton->txl(TXL_SELECT_LANGUAGE), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY(CLAY_ID("LanguageTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(clayton->languageCloseClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }

            CLAY(
                CLAY_ID("LanguageButtons"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 12,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }
            )
            {
                Clay_ElementDeclaration enDecl = LanguageFlagButtonStyle();
                if (clayton->uiLanguage == TXL_LANG_EN_US)
                    enDecl.backgroundColor = CLAY_COLOR_BTN_ACTIVE;
                CLAY(clayton->languageEnglishClick.clayId, enDecl)
                {
                    LanguageFlagButtonContents(clayton, 0, TXL_LANG_EN_US, TXL_LANGUAGE_ENGLISH, buttonFontCfg);
                }

                Clay_ElementDeclaration zhDecl = LanguageFlagButtonStyle();
                if (clayton->uiLanguage == TXL_LANG_ZH_CN)
                    zhDecl.backgroundColor = CLAY_COLOR_BTN_ACTIVE;
                CLAY(clayton->languageChineseClick.clayId, zhDecl)
                {
                    LanguageFlagButtonContents(clayton, 1, TXL_LANG_ZH_CN, TXL_LANGUAGE_CHINESE, buttonFontCfg);
                }

                Clay_ElementDeclaration ltDecl = LanguageFlagButtonStyle();
                if (clayton->uiLanguage == TXL_LANG_LT_LT)
                    ltDecl.backgroundColor = CLAY_COLOR_BTN_ACTIVE;
                CLAY(clayton->languageLithuanianClick.clayId, ltDecl)
                {
                    LanguageFlagButtonContents(clayton, 2, TXL_LANG_LT_LT, TXL_LANGUAGE_LITHUANIAN, buttonFontCfg);
                }

                Clay_ElementDeclaration jpDecl = LanguageFlagButtonStyle();
                if (clayton->uiLanguage == TXL_LANG_JP_JP)
                    jpDecl.backgroundColor = CLAY_COLOR_BTN_ACTIVE;
                CLAY(clayton->languageJapaneseClick.clayId, jpDecl)
                {
                    LanguageFlagButtonContents(clayton, 3, TXL_LANG_JP_JP, TXL_LANGUAGE_JAPANESE, buttonFontCfg);
                }
            }
        }
    }
}
