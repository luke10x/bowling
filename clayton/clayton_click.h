#pragma once

#include <SDL.h>
#include <clay.h>

#define CLAYTON_BUTTON_ID_MAX_LEN 20

struct Clayton_Click
{
    bool isDown;
    Clay_ElementId clayId;
};

void initClaytonClick(Clayton_Click *self, const char *initialId)
{
    char clayIdChar[CLAYTON_BUTTON_ID_MAX_LEN];
    int32_t clayIdCharLen;
    int len = snprintf(clayIdChar, CLAYTON_BUTTON_ID_MAX_LEN, "%s", initialId);
    if (len < 0)
    {
        fprintf(stderr, "Cannot snpintf this: %s\n", initialId);
        abort();
    }
    else if (len < CLAYTON_BUTTON_ID_MAX_LEN)
    {
        clayIdCharLen = len;
    }
    else
    {
        // truncated
        clayIdCharLen = CLAYTON_BUTTON_ID_MAX_LEN - 1;
    }

    Clay_String cs = {.isStaticallyAllocated = false, .chars = clayIdChar, .length = clayIdCharLen};

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