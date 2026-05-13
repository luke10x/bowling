#pragma once

// slider.h
// Reusable touch-friendly horizontal slider for Clay UI.
//
// Usage:
// 1) Declare a `Clayton_Slider` in your state.
// 2) Call `ClaytonSlider_Init(&slider, "MySliderId", min, max, initial)`.
// 3) Each frame:
//    - `ClaytonSlider_ProcessEvent(&slider, e)` to update value on drag.
//    - `ClaytonSlider_Render(&slider, clayton, label, unit)` to draw.

#include <SDL.h>
#include <algorithm>
#include <cmath>

#include "clayton_click.h"
#include "claytheme.h"

struct Clayton_Slider
{
    Clayton_Click container;
    Clayton_Click track;
    Clayton_Click knob;

    float minValue = 0.0f;
    float maxValue = 1.0f;
    float value = 0.0f;

    bool dragging = false;
};

inline void ClaytonSlider_Init(
    Clayton_Slider *self,
    const char *baseId,
    float minValue,
    float maxValue,
    float initialValue
)
{
    if (!self)
        return;

    self->minValue = minValue;
    self->maxValue = maxValue;
    self->value = std::clamp(initialValue, minValue, maxValue);
    self->dragging = false;

    // Stable Clay ids
    char buf[96];
    snprintf(buf, sizeof(buf), "%s_container", baseId);
    initClaytonClick(&self->container, buf);
    snprintf(buf, sizeof(buf), "%s_track", baseId);
    initClaytonClick(&self->track, buf);
    snprintf(buf, sizeof(buf), "%s_knob", baseId);
    initClaytonClick(&self->knob, buf);
}

inline void ClaytonSlider_SetValue(Clayton_Slider *self, float v)
{
    if (!self)
        return;
    self->value = std::clamp(v, self->minValue, self->maxValue);
}

inline float ClaytonSlider_GetT(const Clayton_Slider *self)
{
    if (!self)
        return 0.0f;
    const float denom = (self->maxValue - self->minValue);
    if (denom <= 1e-6f)
        return 0.0f;
    return std::clamp((self->value - self->minValue) / denom, 0.0f, 1.0f);
}

inline void ClaytonSlider_SetFromPointerX(Clayton_Slider *self, float x)
{
    if (!self)
        return;

    Clay_BoundingBox box = Clay_GetElementData(self->track.clayId).boundingBox;
    if (box.width <= 1.0f)
        return;

    // Keep the knob inside the track.
    const float knobW = 34.0f;
    const float left = box.x + knobW * 0.5f;
    const float right = box.x + box.width - knobW * 0.5f;
    float t = (x - left) / (right - left);
    t = std::clamp(t, 0.0f, 1.0f);
    self->value = self->minValue + t * (self->maxValue - self->minValue);
}

inline bool ClaytonSlider_ProcessEvent(Clayton_Slider *self, SDL_Event e)
{
    if (!self)
        return false;

    const bool down = (e.type == SDL_MOUSEBUTTONDOWN);
    const bool up = (e.type == SDL_MOUSEBUTTONUP);
    const bool move = (e.type == SDL_MOUSEMOTION);

    if (down)
    {
        if (Clay_PointerOver(self->container.clayId) || Clay_PointerOver(self->track.clayId) ||
            Clay_PointerOver(self->knob.clayId))
        {
            self->dragging = true;
            ClaytonSlider_SetFromPointerX(self, (float)e.button.x);
            return true;
        }
    }
    if (up)
    {
        if (self->dragging)
        {
            self->dragging = false;
            return true;
        }
    }
    if (move)
    {
        if (self->dragging)
        {
            ClaytonSlider_SetFromPointerX(self, (float)e.motion.x);
            return true;
        }
    }

    // Consume pointer events over slider to prevent click-through.
    if (Clay_PointerOver(self->container.clayId))
    {
        if (down || up || move)
            return true;
    }

    return false;
}

inline void ClaytonSlider_Render(
    Clayton_Slider *self,
    Clayton *clayton,
    const char *label,
    const char *unit
)
{
    if (!self || !clayton)
        return;

    Clay_TextElementConfig labelCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;

    ClayArena *arena = &clayton->clayArena;
    float t = ClaytonSlider_GetT(self);

    CLAY(
        self->container.clayId,
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .childGap = 10,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
        }
    )
    {
        // Label row
        CLAY(
            CLAY_ID("SliderLabelRow"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
            }
        )
        {
            if (label)
                CLAY_TEXT(ClayArena_AllocString(arena, label), CLAY_TEXT_CONFIG(labelCfg));
            CLAY(CLAY_ID("SliderLabelSpacer"), {.layout = {.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}}) {}
            Clay_String v = ClayArena_FormatString(arena, "%.2f%s%s", self->value, (unit ? " " : ""), (unit ? unit : ""));
            CLAY_TEXT(v, CLAY_TEXT_CONFIG(bodyCfg));
        }

    // Track + knob
    CLAY(
        self->track.clayId,
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(44)},
                    .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            .backgroundColor = (Clay_Color){30, 30, 45, 255},
            .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            CLAY_THEME_BTN_BORDER_SMALL
        }
    )
    {
        // We can't lay out "fill + fixed knob" with pure percent sizing without the knob
        // sticking out at t==1. Instead, fill is a background strip and the knob is floated
        // within the track.

        // Filled portion (background strip)
        CLAY(
            CLAY_ID("SliderFill"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(t), CLAY_SIZING_GROW()},
                    },
                .backgroundColor = CLAY_COLOR_BTN_ACTIVE,
                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
            }
        )
        {
        }

        // Knob - float relative to the track so it always stays inside.
        Clay_BoundingBox trackBox = Clay_GetElementData(self->track.clayId).boundingBox;
        const float knobW = 44.0f;
        float x = 0.0f;
        if (trackBox.width > knobW + 1.0f)
        {
            x = t * (trackBox.width - knobW);
        }
        CLAY(
            self->knob.clayId,
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_FIXED(knobW), CLAY_SIZING_FIXED(knobW)},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    },
                .backgroundColor = CLAY_COLOR_BTN_PRIMARY,
                .cornerRadius = {CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG},
                .floating = {
                    .offset = {x, 0.0f},
                    .zIndex = 1,
                    .attachPoints = {CLAY_ATTACH_POINT_LEFT_CENTER, CLAY_ATTACH_POINT_LEFT_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
                .border = {
                    .color = CLAY_COLOR_BORDER,
                    .width = CLAY_BORDER_ALL(1),
                },
            }
        )
        {
        }
    }
}
}
