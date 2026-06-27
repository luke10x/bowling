#pragma once

// Modal UI pause policy:
// - Opening any winstack modal during gameplay must act like a pure pause.
// - It may release transient input-capture state (relative mouse mode, held pointer flags, NOS hold),
//   but it must not reset phase, teleport the ball, or cancel player/enemy throws.
// - Closing the modal should therefore resume from the exact gameplay state that was active before
//   the window opened.
static inline bool UiModalPauseShouldBegin(bool modalWasActiveLastFrame, int windowCount)
{
    return !modalWasActiveLastFrame && windowCount > 0;
}
