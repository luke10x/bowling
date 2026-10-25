#pragma once

#include "clayton.h"
#include "claytheme.h"

inline Clay_ElementDeclaration LanguageFlagButtonStyle()
{
    Clay_ElementDeclaration button = CLAY_THEME_BTN_HUD;
    button.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
    button.layout.childGap = 0;
    button.layout.padding.left = 12;
    button.layout.padding.right = 12;
    button.layout.childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER};
    return button;
}

inline void LanguageFlagButtonContents(
    Clayton *clayton, int slot, TxlLanguage language, TxlKey label,
    Clay_TextElementConfig textConfig)
{
    // Equal side cells keep the label at the button's true center while the
    // flag stays against the left edge. The right cell intentionally stays empty.
    constexpr float kSideCellWidth = 46.0f;
    constexpr float kFlagSize = 34.0f;
    CLAY(CLAY_IDI("LanguageFlagLeft", slot),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(kSideCellWidth), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER}}})
    {
        CLAY(CLAY_IDI("LanguageFlagImage", slot),
             {.layout = {.sizing = {CLAY_SIZING_FIXED(kFlagSize), CLAY_SIZING_FIXED(kFlagSize)}},
              .image = {.imageData = &clayton->languageFlagImages[language]},
              .cornerRadius = {CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM}}) {}
    }
    CLAY(CLAY_IDI("LanguageFlagLabel", slot),
         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW()},
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}}})
    {
        CLAY_TEXT(clayton->txl(label), CLAY_TEXT_CONFIG(textConfig));
    }
    CLAY(CLAY_IDI("LanguageFlagRight", slot),
         {.layout = {.sizing = {CLAY_SIZING_FIXED(kSideCellWidth), CLAY_SIZING_GROW()}}}) {}
}
