#pragma once

#include <SDL.h>
#include <clay.h>

#define CLAYTON_BUTTON_ID_MAX_LEN 80

struct Clayton_Click
{
    bool isDown;
    uint64_t downAtMs;
    Clay_ElementId clayId;

    char idStorage[CLAYTON_BUTTON_ID_MAX_LEN];
    int32_t idLength;
};

void initClaytonClick(Clayton_Click *self, const char *initialId)
{
    if (!initialId)
    {
        fprintf(stderr, "initialId is NULL\n");
        abort();
    }

    int written = snprintf(self->idStorage, CLAYTON_BUTTON_ID_MAX_LEN, "%s", initialId);

    if (written < 0)
    {
        fprintf(stderr, "snprintf failed for: %s\n", initialId);
        abort();
    }

    // Clamp safely
    self->idLength = (written >= CLAYTON_BUTTON_ID_MAX_LEN)
        ? (CLAYTON_BUTTON_ID_MAX_LEN - 1)
        : written;

    // Ensure null termination just in case (debug safety)
    self->idStorage[self->idLength] = '\0';

    Clay_String cs = {
        .isStaticallyAllocated = true,
        .length = self->idLength,
        .chars = self->idStorage,
    };

    self->clayId = CLAY_SID(cs);
    self->isDown = false;
    self->downAtMs = 0;
}

void initClaytonClickIni(Clayton_Click *self, const char *initialId, int i)
{
    if (!initialId)
    {
        fprintf(stderr, "initialId is NULL\n");
        abort();
    }

    int written = snprintf(self->idStorage, CLAYTON_BUTTON_ID_MAX_LEN, "%s", initialId);

    if (written < 0)
    {
        fprintf(stderr, "snprintf failed for: %s\n", initialId);
        abort();
    }

    // Clamp safely
    self->idLength = (written >= CLAYTON_BUTTON_ID_MAX_LEN)
        ? (CLAYTON_BUTTON_ID_MAX_LEN - 1)
        : written;

    // Ensure null termination just in case (debug safety)
    self->idStorage[self->idLength] = '\0';

    Clay_String cs = {
        .isStaticallyAllocated = true,
        .length = self->idLength,
        .chars = self->idStorage,
    };

    self->clayId = CLAY_SIDI(cs, i);
    self->isDown = false;
    self->downAtMs = 0;
}

void initClaytonClickChar(Clayton_Click *self, const char *initialChar)
{
    Clay_String cs = {
        .isStaticallyAllocated = false,
        .length = 1,
        .chars = initialChar,
    };
    self->clayId = CLAY_SID(cs);
    self->isDown = false;
    self->downAtMs = 0;
}

enum Clayton_ClickResult
{
    CLAYTON_CLICK_NONE = 0,
    CLAYTON_CLICK_SHORT = 1,
    CLAYTON_CLICK_LONG = 2,
};

inline Clayton_ClickResult claytonClickReleaseWithHover(
    Clayton_Click *self,
    SDL_Event event,
    bool isHover,
    uint64_t longClickMs)
{
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    bool mouseMove = event.type == SDL_MOUSEMOTION;

    if (self->isDown)
    {
        if (mouseMove && !isHover)
        {
            self->isDown = false;
            self->downAtMs = 0;
            return CLAYTON_CLICK_NONE;
        }
        if (mouseUp)
        {
            self->isDown = false;
            uint64_t elapsed = self->downAtMs > 0 ? SDL_GetTicks64() - self->downAtMs : 0;
            self->downAtMs = 0;
            return elapsed >= longClickMs ? CLAYTON_CLICK_LONG : CLAYTON_CLICK_SHORT;
        }
    }
    else if (mouseDown && isHover)
    {
        self->isDown = true;
        self->downAtMs = SDL_GetTicks64();
    }

    return CLAYTON_CLICK_NONE;
}

inline bool isClaytonClickedWithHover(Clayton_Click *self, SDL_Event event, bool isHover)
{
    // Selector buttons should go through this press/release state machine instead of
    // triggering directly from hover on mouse-up. On touch-capable web builds we can
    // receive both native mouse events and SDL_TOUCH_MOUSEID synthetic mouse events for
    // one physical tap; this path collapses that overlap into one logical click.
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    bool mouseMove = event.type == SDL_MOUSEMOTION;

    if (self->isDown)
    {
        if (mouseMove && !isHover)
        {
            self->isDown = false;
            return false;
        }
        if (mouseUp)
        {
            self->isDown = false;
            self->downAtMs = 0;
            return true; // Yes, clicked if it gets here
        }
    }
    else
    {
        if (mouseDown && isHover)
        {
            self->isDown = true;
            self->downAtMs = SDL_GetTicks64();
            return false;
        }
    }

    return false;
}

bool isClaytonClicked(Clayton_Click *self, SDL_Event event)
{
    return isClaytonClickedWithHover(self, event, Clay_PointerOver(self->clayId));
}
