#pragma once

#include <SDL.h>
#include <clay.h>

#include "clayton_click.h"
#include "claytheme.h"
#include "../storage.h"

#define KEYPAD_ROWS 6
#define KEYPAD_COLS 6

static char KEYPAD_DEFAULT_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', '4', '5', '6'},
    {'7', '8', '9', '0', 'E', 'T'},
    {'A', 'O', 'I', 'N', 'S', 'R'},
    {'H', 'L', 'D', 'C', 'U', 'M'},
    {'F', 'Y', 'W', 'G', 'P', 'B'},
    {'V', 'K', 'X', 'Q', 'J', 'Z'},
};

#define KEYPAD_MAX_CHARS 12
struct Keypad
{
    char *originalText;
    int32_t *originalTextLen;
    const char *title; // UI label for the current text-entry session (string literal or other long-lived storage)
    char currentText[KEYPAD_MAX_CHARS];
    int32_t currentTextLen;
    bool activated;
    char keys[KEYPAD_ROWS][KEYPAD_COLS];
    Clayton_Click clicks[KEYPAD_ROWS][KEYPAD_COLS];
    Clayton_Click delClick;
    Clayton_Click spaceClick;
    Clayton_Click enterClick;
    Clayton_Click closeClick;
    bool newsDetected;
};

inline void buildKeypadWindowClay(Keypad *self);

void initKeypad(Keypad *self, char *originalText, int32_t *originalTextLen)
{
    self->originalText = originalText;
    self->originalTextLen = originalTextLen;
    self->title = "Enter Text";
    self->activated = false;

    for (int i = 0; i < KEYPAD_ROWS; ++i)
    {
        memcpy(self->keys[i], KEYPAD_DEFAULT_KEYS[i], KEYPAD_COLS);

        for (int j = 0; j < KEYPAD_COLS; ++j)
        {
            initClaytonClickChar(&self->clicks[i][j], &self->keys[i][j]);
        }
    }
    initClaytonClick(&self->delClick, "deleteClick");
    initClaytonClick(&self->spaceClick, "spaceClick");
    initClaytonClick(&self->enterClick, "enterClick");
    initClaytonClick(&self->closeClick, "closeClick");

    self->newsDetected = false;
}

void uploadKeypadText(Keypad *self)
{
    size_t toCopy = *self->originalTextLen;
    if (toCopy >= KEYPAD_MAX_CHARS)
    {
        toCopy = KEYPAD_MAX_CHARS - 1;
    }
    memcpy(self->currentText, self->originalText, toCopy);

    // self->currentText[toCopy] = '\0';

    self->currentTextLen = toCopy;
}

bool processKeypadEvent(Keypad *self, SDL_Event event, Storage *storage)
{
    if (!Clay_PointerOver(CLAY_ID("KeypadContainer")))
    {
        return false;
    }
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    bool mouseMove = event.type == SDL_MOUSEMOTION;

    if (!mouseDown && !mouseUp && !mouseMove)
    {
        // TODO maybe keyboard or joystick
        return false;
    }

    for (int i = 0; i < KEYPAD_ROWS; ++i)
    {
        for (int j = 0; j < KEYPAD_COLS; ++j)
        {
            char buf[20];
            if (isClaytonClicked(&self->clicks[i][j], event))
            {
                if (self->currentTextLen < KEYPAD_MAX_CHARS)
                {
                    self->currentText[self->currentTextLen] = self->keys[i][j];
                    self->currentTextLen += 1;
                }
                else
                {
                    // TODO visual bell
                }
            }
        }
    }
    if (isClaytonClicked(&self->delClick, event))
    {
        if (self->currentTextLen > 0)
        {
            self->currentTextLen -= 1;
        }
    }
    if (isClaytonClicked(&self->spaceClick, event))
    {
        if (self->currentTextLen < KEYPAD_MAX_CHARS)
        {
            self->currentText[self->currentTextLen] = '_';
            self->currentTextLen += 1;
        }
    }
    if (isClaytonClicked(&self->enterClick, event))
    {
        {
            size_t toCopy = self->currentTextLen;

            memcpy(self->originalText, self->currentText, toCopy);

            *self->originalTextLen = toCopy;
            storage->setChar(Storage::USERNAME, self->originalText, toCopy);
        }
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

void buildKeypadClay(Keypad *self)
{
    if (!self->activated)
    {
        return;
    }

    Clay_TextElementConfig keyFontCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig inputFontCfg = CLAY_THEME_TEXT_INPUT;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;

    // Legacy wrapper: keeps existing behavior for call sites that expect this function to provide
    // its own full-screen overlay.
    CLAY(CLAY_ID("KeypadContainerOverlay"), CLAY_THEME_OVERLAY)
    {
        buildKeypadWindowClay(self);
    }
}

// Content-only keypad window (no dim overlay). Used by WindowStack to layer overlays between windows.
inline void buildKeypadWindowClay(Keypad *self)
{
    if (!self || !self->activated)
    {
        return;
    }

    Clay_TextElementConfig keyFontCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig inputFontCfg = CLAY_THEME_TEXT_INPUT;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;

    // Root container exists for pointer-hit testing in processKeypadEvent().
    CLAY(
        CLAY_ID("KeypadContainer"),
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
        CLAY(
            CLAY_ID("KeypadLittleWindow"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(0.9), CLAY_SIZING_FIT()},
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
                CLAY_ID("KeypadTitle"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 0, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                CLAY(
                    CLAY_ID("KeypadTitleWrapper"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .padding = {0, 0, 0, 0},
                                .childGap = 10,
                                .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                    }
                ) {

                    const char *title =
                        (self->title && self->title[0]) ? self->title : "Enter Text";
                    Clay_String titleStr = {
                        .isStaticallyAllocated = false,
                        .length = (int)strlen(title),
                        .chars = (char *)title,
                    };
                    CLAY_TEXT(titleStr, CLAY_TEXT_CONFIG(titleFontCfg));

                }

                CLAY(
                    self->closeClick.clayId,
                    CLAY_THEME_BTN_DANGER
                ) {
                    CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }
            CLAY(
                CLAY_ID("FirstRowForInput"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 0, 0},
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                CLAY(
                    CLAY_ID("Input border"),
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
                    Clay_String cs = Clay_String{
                        .isStaticallyAllocated = false,
                        .length = self->currentTextLen,
                        .chars = self->currentText,
                    };
                    CLAY_TEXT(cs, CLAY_TEXT_CONFIG(inputFontCfg));
                }
            }
            for (int row = 0; row < KEYPAD_ROWS; row++)
            {
                CLAY_AUTO_ID({
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 0, 0},
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                })
                {
                    for (int col = 0; col < KEYPAD_COLS; col++)
                    {
                        CLAY(
                            self->clicks[row][col].clayId,
                            {
                                .layout =
                                    {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childAlignment =
                                    {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                                .aspectRatio = {1.0f},
                            }
                        )
                        {

                            Clay_String cs = Clay_String{
                                .isStaticallyAllocated = false,
                                .length = 1,
                                .chars = &self->keys[row][col],
                            };
                            CLAY_TEXT(cs, CLAY_TEXT_CONFIG(keyFontCfg));
                        };
                    } // end of for col
                } // end of row clay element
            } // end of for row

            CLAY(
                CLAY_ID("LastRow"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 0, 0},
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    .backgroundColor = CLAY_COLOR_PANEL_BG,
                }
            )
            {
                CLAY(
                    self->delClick.clayId,
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {2.0f},
                    }
                )
                {

                    CLAY_TEXT(CLAY_STRING("Delete"), CLAY_TEXT_CONFIG(keyFontCfg));
                }
                CLAY(
                    self->spaceClick.clayId,
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {2.0f},
                    }
                )
                {

                    CLAY_TEXT(CLAY_STRING("Stress"), CLAY_TEXT_CONFIG(keyFontCfg));
                }
                CLAY(
                    self->enterClick.clayId,
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                        .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                        .aspectRatio = {2.0f},
                    }
                )
                {
                    CLAY_TEXT(CLAY_STRING("Enter"), CLAY_TEXT_CONFIG(keyFontCfg));
                }

            } // End of last row
        }; // end of kepad window
    } // end of outter container
}
