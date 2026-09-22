#pragma once

#include <SDL.h>
#include <clay.h>

#include <stdint.h>
#include <string.h>

#include "clayton_click.h"
#include "claytheme.h"
#include "../tegel/txl_runtime.h"

#define NUMKEYPAD_COLS 3
#define NUMKEYPAD_MAX_CHARS 16
#define NUMKEYPAD_MINUS_LONG_CLICK_MS 450

static char NUMKEYPAD_DIGIT_KEYS[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

enum NumKeypadBase
{
    NUMKEYPAD_BASE_DECIMAL = 10,
    NUMKEYPAD_BASE_HEX = 16,
};

struct NumKeypadRules
{
    int32_t minValue;
    int32_t maxValue;
    int32_t base;
    bool allowZeroValue;
    const int32_t *allowedValues;
    int32_t allowedValueCount;
};

struct NumKeypad
{
    int32_t *originalValue;
    const char *title;
    TxlLanguage uiLanguage;
    NumKeypadRules rules;
    char currentText[NUMKEYPAD_MAX_CHARS];
    int32_t currentTextLen;
    bool activated;
    bool newsDetected;
    Clayton_Click keyClicks[16];
    Clayton_Click delClick;
    Clayton_Click enterClick;
    Clayton_Click closeClick;
    Clayton_Click baseToggleClick;
};

inline Clay_String NumKeypad_TxlString(TxlLanguage language, TxlKey key)
{
    const char *text = Txl_Get(language, key);
    return {
        .isStaticallyAllocated = true,
        .length = (int32_t)strlen(text),
        .chars = text,
    };
}

inline int32_t NumKeypad_NormalizedBase(int32_t base)
{
    return base == NUMKEYPAD_BASE_HEX ? NUMKEYPAD_BASE_HEX : NUMKEYPAD_BASE_DECIMAL;
}

inline char NumKeypad_DigitChar(int32_t digit)
{
    return digit < 10 ? (char)('0' + digit) : (char)('A' + (digit - 10));
}

inline int32_t NumKeypad_DigitValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    return -1;
}

inline bool NumKeypad_HasNegativePrefix(const char *text, int32_t textLen)
{
    return text && textLen > 0 && text[0] == '-';
}

inline bool NumKeypad_ParseText(
    const char *text,
    int32_t textLen,
    int32_t base,
    int64_t *outValue
)
{
    if (!text || textLen <= 0)
        return false;

    int64_t value = 0;
    const int32_t normalizedBase = NumKeypad_NormalizedBase(base);
    const bool negative = NumKeypad_HasNegativePrefix(text, textLen);
    const int32_t firstDigit = negative ? 1 : 0;
    if (firstDigit >= textLen)
        return false;

    for (int32_t i = firstDigit; i < textLen; ++i)
    {
        const int32_t digit = NumKeypad_DigitValue(text[i]);
        if (digit < 0 || digit >= normalizedBase)
            return false;

        if (value > (INT64_MAX - digit) / normalizedBase)
            return false;
        value = value * normalizedBase + digit;
    }

    if (outValue)
        *outValue = negative ? -value : value;
    return true;
}

inline int32_t NumKeypad_FormatValueText(int32_t value, int32_t base, char *outText, int32_t outCapacity)
{
    if (!outText || outCapacity <= 0)
        return 0;

    const int32_t normalizedBase = NumKeypad_NormalizedBase(base);
    char buffer[NUMKEYPAD_MAX_CHARS] = {};
    int32_t len = 0;
    int64_t cursor = value < 0 ? -(int64_t)value : (int64_t)value;
    do
    {
        const int32_t digit = (int32_t)(cursor % normalizedBase);
        buffer[len++] = NumKeypad_DigitChar(digit);
        cursor /= normalizedBase;
    } while (cursor > 0 && len < NUMKEYPAD_MAX_CHARS);

    int32_t outLen = 0;
    if (value < 0 && outLen < outCapacity)
        outText[outLen++] = '-';
    for (int32_t i = 0; i < len && outLen < outCapacity; ++i)
        outText[outLen++] = buffer[len - 1 - i];
    return outLen;
}

inline bool NumKeypad_TextHasPrefix(const char *text, int32_t textLen, const char *candidate, int32_t candidateLen)
{
    return text && candidate && textLen <= candidateLen && memcmp(text, candidate, textLen) == 0;
}

inline bool NumKeypad_DiscreteTextCanLeadToValidValue(
    NumKeypadRules rules,
    const char *text,
    int32_t textLen
)
{
    if (!rules.allowedValues || rules.allowedValueCount <= 0)
        return false;

    rules.base = NumKeypad_NormalizedBase(rules.base);
    for (int32_t i = 0; i < rules.allowedValueCount; ++i)
    {
        const int32_t value = rules.allowedValues[i];
        if (value < rules.minValue || value > rules.maxValue)
            continue;
        if (value == 0 && !rules.allowZeroValue)
            continue;

        char candidate[NUMKEYPAD_MAX_CHARS] = {};
        const int32_t candidateLen = NumKeypad_FormatValueText(
            value,
            rules.base,
            candidate,
            NUMKEYPAD_MAX_CHARS
        );
        if (NumKeypad_TextHasPrefix(text, textLen, candidate, candidateLen))
            return true;
    }

    return false;
}

inline bool NumKeypad_TextCanLeadToValidValue(
    NumKeypadRules rules,
    const char *text,
    int32_t textLen,
    int32_t maxTextLen = NUMKEYPAD_MAX_CHARS
)
{
    if (!text || textLen <= 0 || textLen > maxTextLen || rules.minValue > rules.maxValue)
        return false;

    rules.base = NumKeypad_NormalizedBase(rules.base);
    if (rules.allowedValues && rules.allowedValueCount > 0)
        return NumKeypad_DiscreteTextCanLeadToValidValue(rules, text, textLen);

    const bool negative = NumKeypad_HasNegativePrefix(text, textLen);
    if (negative)
    {
        if (rules.minValue >= 0)
            return false;
        if (textLen == 1)
            return textLen < maxTextLen;
        if (text[1] == '0')
            return false;
    }
    else if (text[0] == '0')
    {
        return textLen == 1 && rules.allowZeroValue && rules.minValue <= 0 && rules.maxValue >= 0;
    }

    int64_t parsedValue = 0;
    if (!NumKeypad_ParseText(text, textLen, rules.base, &parsedValue))
        return false;

    const int64_t prefixValue = negative ? -parsedValue : parsedValue;

    int64_t scale = 1;
    for (int32_t extraDigits = 0; textLen + extraDigits <= maxTextLen; ++extraDigits)
    {
        if (prefixValue > INT64_MAX / scale)
            return false;

        const int64_t low = prefixValue * scale;
        const int64_t high =
            (scale - 1 > INT64_MAX - low) ? INT64_MAX : low + scale - 1;
        if (negative)
        {
            const int64_t valueLow = -high;
            const int64_t valueHigh = -low;
            if (valueHigh < rules.minValue)
                return false;
            if (valueLow <= rules.maxValue && valueHigh >= rules.minValue)
                return true;
        }
        else
        {
            if (low > rules.maxValue)
                return false;
            if (high >= rules.minValue && low <= rules.maxValue)
                return true;
        }

        if (scale > INT64_MAX / rules.base)
            return false;
        if (prefixValue > INT64_MAX / (scale * rules.base))
            return false;
        scale *= rules.base;
    }

    return false;
}

inline bool NumKeypad_CanAppendDigit(
    NumKeypadRules rules,
    const char *text,
    int32_t textLen,
    int32_t digit
)
{
    rules.base = NumKeypad_NormalizedBase(rules.base);
    if (!text || textLen < 0 || textLen >= NUMKEYPAD_MAX_CHARS || digit < 0 || digit >= rules.base)
        return false;

    char nextText[NUMKEYPAD_MAX_CHARS] = {};
    if (textLen > 0)
        memcpy(nextText, text, textLen);
    nextText[textLen] = NumKeypad_DigitChar(digit);
    return NumKeypad_TextCanLeadToValidValue(rules, nextText, textLen + 1);
}

inline bool NumKeypad_CanAppendMinus(
    NumKeypadRules rules,
    const char *text,
    int32_t textLen
)
{
    if (!text || textLen != 0 || rules.minValue >= 0 || textLen >= NUMKEYPAD_MAX_CHARS)
        return false;

    const char minusText[1] = {'-'};
    return NumKeypad_TextCanLeadToValidValue(rules, minusText, 1);
}

inline bool NumKeypad_DiscreteCanEnter(NumKeypadRules rules, int64_t value)
{
    if (!rules.allowedValues || rules.allowedValueCount <= 0)
        return true;

    for (int32_t i = 0; i < rules.allowedValueCount; ++i)
    {
        if ((int64_t)rules.allowedValues[i] == value)
            return true;
    }
    return false;
}

inline bool NumKeypad_CanEnter(NumKeypadRules rules, const char *text, int32_t textLen)
{
    if (!text || textLen <= 0 || rules.minValue > rules.maxValue)
        return false;

    rules.base = NumKeypad_NormalizedBase(rules.base);
    const bool negative = NumKeypad_HasNegativePrefix(text, textLen);
    if (negative && textLen == 1)
        return false;
    if (negative && text[1] == '0')
        return false;
    if (!negative && text[0] == '0' && (textLen > 1 || !rules.allowZeroValue))
        return false;

    int64_t value = 0;
    if (!NumKeypad_ParseText(text, textLen, rules.base, &value))
        return false;

    return value >= rules.minValue && value <= rules.maxValue && NumKeypad_DiscreteCanEnter(rules, value);
}

inline int32_t NumKeypad_CurrentValue(const NumKeypad *self)
{
    int64_t value = 0;
    if (!self || !NumKeypad_ParseText(
        self->currentText,
        self->currentTextLen,
        self->rules.base,
        &value
    ))
    {
        return 0;
    }
    return (int32_t)value;
}

inline void NumKeypad_ReformatCurrentValue(NumKeypad *self, int32_t nextBase)
{
    if (!self)
        return;

    int64_t value = 0;
    const bool hadValidValue = NumKeypad_ParseText(
        self->currentText,
        self->currentTextLen,
        self->rules.base,
        &value
    );

    self->rules.base = NumKeypad_NormalizedBase(nextBase);
    self->currentTextLen = 0;
    if (!hadValidValue || value < self->rules.minValue || value > self->rules.maxValue)
        return;
    if (value == 0 && !self->rules.allowZeroValue)
        return;

    self->currentTextLen = NumKeypad_FormatValueText(
        (int32_t)value,
        self->rules.base,
        self->currentText,
        NUMKEYPAD_MAX_CHARS
    );
}

inline void NumKeypad_ToggleBase(NumKeypad *self)
{
    if (!self)
        return;
    NumKeypad_ReformatCurrentValue(
        self,
        NumKeypad_NormalizedBase(self->rules.base) == NUMKEYPAD_BASE_HEX ?
            NUMKEYPAD_BASE_DECIMAL :
            NUMKEYPAD_BASE_HEX
    );
}

inline void initNumKeypad(
    NumKeypad *self,
    int32_t *originalValue,
    int32_t minValue,
    int32_t maxValue,
    int32_t base = NUMKEYPAD_BASE_DECIMAL,
    bool allowZeroValue = false
)
{
    self->originalValue = originalValue;
    self->title = nullptr;
    self->uiLanguage = TXL_LANG_EN_US;
    self->rules = {
        .minValue = minValue,
        .maxValue = maxValue,
        .base = NumKeypad_NormalizedBase(base),
        .allowZeroValue = allowZeroValue,
        .allowedValues = nullptr,
        .allowedValueCount = 0,
    };
    self->currentTextLen = 0;
    self->activated = false;
    self->newsDetected = false;

    for (int32_t i = 0; i < 16; ++i)
        initClaytonClickIni(&self->keyClicks[i], "numKeypadKey", i);
    initClaytonClick(&self->delClick, "numKeypadDelete");
    initClaytonClick(&self->enterClick, "numKeypadEnter");
    initClaytonClick(&self->closeClick, "numKeypadClose");
    initClaytonClick(&self->baseToggleClick, "numKeypadBaseToggle");
}

inline void uploadNumKeypadValue(NumKeypad *self)
{
    if (!self)
        return;

    self->currentTextLen = 0;
    int32_t value = self->originalValue ? *self->originalValue : self->rules.minValue;
    if (value < self->rules.minValue || value > self->rules.maxValue)
        return;
    if (value == 0 && !self->rules.allowZeroValue)
        return;

    self->currentTextLen = NumKeypad_FormatValueText(
        value,
        self->rules.base,
        self->currentText,
        NUMKEYPAD_MAX_CHARS
    );
}

inline bool NumKeypad_CanAppend(const NumKeypad *self, int32_t digit)
{
    return self && NumKeypad_CanAppendDigit(
        self->rules,
        self->currentText,
        self->currentTextLen,
        digit
    );
}

inline bool NumKeypad_CanSubmit(const NumKeypad *self)
{
    return self && NumKeypad_CanEnter(self->rules, self->currentText, self->currentTextLen);
}

inline bool NumKeypad_CanAppendMinus(const NumKeypad *self)
{
    return self && NumKeypad_CanAppendMinus(self->rules, self->currentText, self->currentTextLen);
}

inline bool processNumKeypadEvent(NumKeypad *self, SDL_Event event)
{
    if (!self || !self->activated || !Clay_PointerOver(CLAY_ID("NumKeypadContainer")))
        return false;

    const bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    const bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    const bool mouseMove = event.type == SDL_MOUSEMOTION;
    if (!mouseDown && !mouseUp && !mouseMove)
        return false;

    const int32_t base = NumKeypad_NormalizedBase(self->rules.base);
    const bool zeroOrMinusEnabled = NumKeypad_CanAppend(self, 0) || NumKeypad_CanAppendMinus(self);
    if (zeroOrMinusEnabled)
    {
        const Clayton_ClickResult zeroClick = claytonClickReleaseWithHover(
            &self->keyClicks[0],
            event,
            Clay_PointerOver(self->keyClicks[0].clayId),
            NUMKEYPAD_MINUS_LONG_CLICK_MS
        );
        if (zeroClick == CLAYTON_CLICK_LONG && NumKeypad_CanAppendMinus(self))
        {
            self->currentText[self->currentTextLen] = '-';
            self->currentTextLen += 1;
        }
        else if (zeroClick == CLAYTON_CLICK_SHORT && NumKeypad_CanAppend(self, 0))
        {
            self->currentText[self->currentTextLen] = '0';
            self->currentTextLen += 1;
        }
    }

    for (int32_t digit = 1; digit < base; ++digit)
    {
        if (NumKeypad_CanAppend(self, digit) && isClaytonClicked(&self->keyClicks[digit], event))
        {
            self->currentText[self->currentTextLen] = NumKeypad_DigitChar(digit);
            self->currentTextLen += 1;
        }
    }

    if (isClaytonClicked(&self->delClick, event) && self->currentTextLen > 0)
        self->currentTextLen -= 1;

    if (isClaytonClicked(&self->baseToggleClick, event))
        NumKeypad_ToggleBase(self);

    if (NumKeypad_CanSubmit(self) && isClaytonClicked(&self->enterClick, event))
    {
        if (self->originalValue)
            *self->originalValue = NumKeypad_CurrentValue(self);
        self->activated = false;
        self->newsDetected = true;
    }

    if (isClaytonClicked(&self->closeClick, event))
    {
        self->activated = false;
        self->newsDetected = false;
    }

    return true;
}

inline void buildNumKeypadWindowClay(NumKeypad *self);

inline void buildNumKeypadClay(NumKeypad *self)
{
    if (!self || !self->activated)
        return;

    CLAY(CLAY_ID("NumKeypadContainerOverlay"), CLAY_THEME_OVERLAY)
    {
        buildNumKeypadWindowClay(self);
    }
}

inline Clay_Color NumKeypad_ButtonColor(Clay_ElementId clayId, bool enabled, Clay_Color base)
{
    if (!enabled)
        return CLAY_COLOR_BTN_DISABLED;
    return Clay_PointerOver(clayId) ? ClayTheme_HoverColor(base, 24.0f, 0.0f) : base;
}

inline void buildNumKeypadDigitClay(NumKeypad *self, int32_t digit, Clay_TextElementConfig keyFontCfg)
{
    const bool enabled = digit == 0 ?
        (NumKeypad_CanAppend(self, 0) || NumKeypad_CanAppendMinus(self)) :
        NumKeypad_CanAppend(self, digit);
    CLAY(
        self->keyClicks[digit].clayId,
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                },
            .backgroundColor = NumKeypad_ButtonColor(self->keyClicks[digit].clayId, enabled, CLAY_COLOR_BTN_PRIMARY),
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            .aspectRatio = {2.0f},
            CLAY_THEME_BTN_BORDER_SMALL
        }
    )
    {
        if (digit == 0 && NumKeypad_CanAppendMinus(self))
        {
            CLAY_TEXT(CLAY_STRING("0/-"), CLAY_TEXT_CONFIG(keyFontCfg));
        }
        else
        {
            Clay_String cs = {
                .isStaticallyAllocated = false,
                .length = 1,
                .chars = &NUMKEYPAD_DIGIT_KEYS[digit],
            };
            CLAY_TEXT(cs, CLAY_TEXT_CONFIG(keyFontCfg));
        }
    }
}

inline void buildNumKeypadDigitRowClay(
    NumKeypad *self,
    int32_t firstDigit,
    Clay_TextElementConfig keyFontCfg
)
{
    CLAY_AUTO_ID({
        .layout =
            {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                .childGap = 10,
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        .backgroundColor = CLAY_COLOR_PANEL_BG,
    })
    {
        for (int32_t col = 0; col < NUMKEYPAD_COLS; ++col)
            buildNumKeypadDigitClay(self, firstDigit + col, keyFontCfg);
    }
}

inline void buildNumKeypadWindowClay(NumKeypad *self)
{
    if (!self || !self->activated)
        return;

    Clay_TextElementConfig keyFontCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig inputFontCfg = CLAY_THEME_TEXT_INPUT;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;

    CLAY(
        CLAY_ID("NumKeypadContainer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
        }
    )
    {
        CLAY(
            CLAY_ID("NumKeypadLittleWindow"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(0.9f), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                CLAY_THEME_WINDOW_BORDER
            }
        )
        {
            CLAY(
                CLAY_ID("NumKeypadTitle"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                CLAY(
                    CLAY_ID("NumKeypadTitleWrapper"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                    }
                )
                {
                    const char *title = (self->title && self->title[0]) ? self->title : Txl_Get(self->uiLanguage, TXL_KEYPAD_ENTER_NUMBER);
                    Clay_String titleStr = {
                        .isStaticallyAllocated = false,
                        .length = (int)strlen(title),
                        .chars = (char *)title,
                    };
                    CLAY_TEXT(titleStr, CLAY_TEXT_CONFIG(titleFontCfg));
                }

                Clay_ElementDeclaration baseToggle = CLAY_THEME_BTN_PRIMARY;
                baseToggle.layout.sizing = {CLAY_SIZING_FIXED(86), CLAY_SIZING_FIXED(60)};
                const Clay_Color baseToggleColor =
                    NumKeypad_NormalizedBase(self->rules.base) == NUMKEYPAD_BASE_HEX ?
                        CLAY_COLOR_BTN_SUCCESS :
                        CLAY_COLOR_BTN_PRIMARY;
                baseToggle.backgroundColor = Clay_PointerOver(self->baseToggleClick.clayId) ?
                    ClayTheme_HoverColor(baseToggleColor, 24.0f, 0.0f) :
                    baseToggleColor;
                CLAY(self->baseToggleClick.clayId, baseToggle)
                {
                    CLAY_TEXT(
                        NumKeypad_NormalizedBase(self->rules.base) == NUMKEYPAD_BASE_HEX ?
                            CLAY_STRING("HEX") :
                            CLAY_STRING("DEC"),
                        CLAY_TEXT_CONFIG(buttonFontCfg)
                    );
                }

                CLAY(self->closeClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }

            CLAY(
                CLAY_ID("NumKeypadInputRow"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                CLAY(
                    CLAY_ID("NumKeypadInputBorder"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .padding = {0, 10, 0, 0},
                                .childAlignment = {CLAY_ALIGN_X_RIGHT, CLAY_ALIGN_Y_CENTER},
                            },
                        .backgroundColor = CLAY_COLOR_PANEL_SECTION,
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {6.0f},
                    }
                )
                {
                    Clay_String cs = {
                        .isStaticallyAllocated = false,
                        .length = self->currentTextLen,
                        .chars = self->currentText,
                    };
                    CLAY_TEXT(cs, CLAY_TEXT_CONFIG(inputFontCfg));
                }
            }

            buildNumKeypadDigitRowClay(self, 1, keyFontCfg);
            buildNumKeypadDigitRowClay(self, 4, keyFontCfg);
            buildNumKeypadDigitRowClay(self, 7, keyFontCfg);
            if (NumKeypad_NormalizedBase(self->rules.base) == NUMKEYPAD_BASE_HEX)
            {
                buildNumKeypadDigitRowClay(self, 10, keyFontCfg);
                buildNumKeypadDigitRowClay(self, 13, keyFontCfg);
            }

            CLAY(
                CLAY_ID("NumKeypadLastRow"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                const bool canDelete = self->currentTextLen > 0;
                CLAY(
                    self->delClick.clayId,
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = NumKeypad_ButtonColor(self->delClick.clayId, canDelete, CLAY_COLOR_BTN_PRIMARY),
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {2.0f},
                        CLAY_THEME_BTN_BORDER_SMALL
                    }
                )
                {
                    CLAY_TEXT(NumKeypad_TxlString(self->uiLanguage, TXL_DELETE), CLAY_TEXT_CONFIG(keyFontCfg));
                }

                buildNumKeypadDigitClay(self, 0, keyFontCfg);

                const bool canSubmit = NumKeypad_CanSubmit(self);
                CLAY(
                    self->enterClick.clayId,
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = NumKeypad_ButtonColor(self->enterClick.clayId, canSubmit, CLAY_COLOR_BTN_SUCCESS),
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {2.0f},
                        CLAY_THEME_BTN_BORDER_SMALL
                    }
                )
                {
                    CLAY_TEXT(NumKeypad_TxlString(self->uiLanguage, TXL_ENTER), CLAY_TEXT_CONFIG(keyFontCfg));
                }
            }
        }
    }
}
