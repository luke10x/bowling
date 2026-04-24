#pragma once

#include <SDL.h>
#include <clay.h>

#define CLAYTON_BUTTON_ID_MAX_LEN 80

struct Clayton_Click
{
    bool isDown;
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
}

bool isClaytonClicked(Clayton_Click *self, SDL_Event event)
{
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    bool mouseMove = event.type == SDL_MOUSEMOTION;
    bool isHover = Clay_PointerOver(self->clayId);

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
            return true; // Yes, clicked if it gets here
        }
    }
    else
    {
        if (mouseDown && isHover)
        {
            self->isDown = true;
            return false;
        }
    }

    return false;
}