#pragma once

#include <SDL.h>
#include <clay.h>

struct Keypad
{
    char *originalText;
    size_t *originalTextLen;
    char *currentText;
    size_t currentTextLen;
    bool activated;
};

void initKeypad(Keypad *self, char *originalText, size_t *originalTextLen)
{
    self->originalText = originalText;
    self->originalTextLen = originalTextLen;
    self->activated = false;
}

void processKeypadEvent(Keypad *self, SDL_Event event)
{
    if (!Clay_PointerOver(CLAY_ID("KeypadContainer")))
    {
        return;
    }
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    bool mouseMove = event.type == SDL_MOUSEMOTION;

    if (!mouseDown && !mouseUp && !mouseMove)
    {
        return;
    }

    self->activated = false;
    // if (e.type == SDL_MOUSEBUTTONDOWN)
    // {
    //     mouseClicked = true; // will see this later
    // }
}

void buildKeypadClay(Keypad *self)
{
    if (!self->activated)
    {
        return;
    }

    CLAY(
        CLAY_ID("KeypadContainer"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = {0, 0, 0, 0},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            .backgroundColor = {255, 2, 2, 100},

        }
    )
    {
    }
}