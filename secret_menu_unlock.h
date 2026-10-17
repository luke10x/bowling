#pragma once

#include <cstdint>

// A session-only HUD gesture: charge, elapsed time, then money within two seconds.
// A click counts only when the same pointer presses and releases on the same area.
struct SecretMenuUnlock
{
    enum Area : uint8_t { Other, Energy, Timer, Money };
    static constexpr uint64_t WindowMs = 2000;

    bool unlocked = false;
    uint8_t progress = 0;
    uint64_t firstClickMs = 0;
    Area pressedArea = Other;
    uint64_t pressedPointer = 0;
    bool pressedWasTouch = false;

    void cancel()
    {
        progress = 0;
        pressedArea = Other;
    }

    Area expected() const
    {
        return progress == 1 ? Timer : progress == 2 ? Money : Energy;
    }

    void pointerDown(Area area, bool touch, uint64_t pointer, uint64_t nowMs)
    {
        if (unlocked) return;
        if (progress && (nowMs - firstClickMs > WindowMs || area != expected()))
            progress = 0;
        pressedArea = area;
        pressedWasTouch = touch;
        pressedPointer = pointer;
    }

    bool pointerUp(Area area, bool touch, uint64_t pointer, uint64_t nowMs)
    {
        if (unlocked) return false;
        const bool completeClick = pressedArea != Other && pressedArea == area &&
            pressedWasTouch == touch && pressedPointer == pointer;
        pressedArea = Other;
        if (!completeClick || (progress && nowMs - firstClickMs > WindowMs))
        {
            progress = 0;
            return false;
        }
        if (progress == 0 && area == Energy)
        {
            progress = 1;
            firstClickMs = nowMs;
        }
        else if (progress == 1 && area == Timer)
        {
            progress = 2;
        }
        else if (progress == 2 && area == Money)
        {
            progress = 0;
            unlocked = true;
            return true;
        }
        else
        {
            progress = 0;
        }
        return false;
    }
};
