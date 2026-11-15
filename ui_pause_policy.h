#pragma once

// Modal UI pause policy:
// - Opening a modal while a ball is being aimed or swung, but before release,
//   cancels that unfinished attempt and returns the ball to idle.
// - A released ball remains in its current throw state while modals are open.
static inline bool UiModalPauseShouldBegin(bool modalWasActiveLastFrame, int windowCount)
{
    return !modalWasActiveLastFrame && windowCount > 0;
}

static inline bool UiModalPauseShouldResetUnthrownAim(bool modalJustOpened, bool aimOrSwingActive)
{
    return modalJustOpened && aimOrSwingActive;
}
