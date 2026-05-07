#pragma once

// win_stack.h (module)
// Centralizes "window stack" ordering, topmost-only SDL event routing, and window rendering order.
//
// Intent:
// - Avoid polymorphism; use explicit switch dispatch per window type.
// - Keep window logic searchable (e.g. processKeypadWindowEvent, renderShopWindow).
// - Ensure only the topmost window consumes SDL events; if it consumes, game must not see them.

#include <SDL.h>
#include <stdint.h>

// This module must not depend on the giant context struct defined in game.cpp (not includable).
// Instead, we include the modules we need and pass those pointers explicitly.
#include "clayton.h"
#include "keypad.h"
#include "new_game_clay.h"
#include "shop_clay.h"
#include "../hiscore/localhi.h"
#include "../hiscore/hiscore_clay.h"
#include "../shop.h"
#include "../sounds/adaptive_audio.h"
#include "../sounds/adaptive_clay.h"
#include "../sounds/sounds.h"
#include "../sounds/sound_clay.h"
#include "../oil/oil_clay.h"
#include "../oil/oil_clay.h"

// Keep this small; we statically allocate in WindowStack.
#ifndef WINDOW_STACK_MAX
#define WINDOW_STACK_MAX 8
#endif

enum WindowKind // I like it 
{
    WindowKind_AdaptiveAudio,
    WindowKind_SoundSettings,
    WindowKind_LocalHiscore,
    WindowKind_OilStatus,
    WindowKind_Shop,
    WindowKind_Keypad,
    WindowKind_AudioCacheProgress,
    WindowKind_NewGame,
};

struct WindowStack
{
    WindowKind kinds[WINDOW_STACK_MAX];
    int count;

    // State that previously lived as `static` locals in game.cpp shop event handling.
    // Mirrors `s_shopPointerDown/s_shopLastX/s_shopLastY` from game.cpp to compute drag deltas.
    bool shopPointerDown;
    int shopLastX;
    int shopLastY;
    bool shopBuyRequested;
    bool oilReoilRequested;
    bool playAgainRequested;

    // ---- Public API ----
    inline void windowStackInit()
    {
        count = 0;
        shopPointerDown = false;
        shopLastX = 0;
        shopLastY = 0;
        shopBuyRequested = false;
        oilReoilRequested = false;
        playAgainRequested = false;
    }

    // ---- Push helpers (call sites never mention WindowKind) ----
    inline void windowStackPushAdaptiveAudioWindow()
    {
        windowStackPushWindow_(WindowKind_AdaptiveAudio);
    }
    inline void windowStackPushSoundSettingsWindow()
    {
        windowStackPushWindow_(WindowKind_SoundSettings);
    }
    inline void windowStackPushLocalHiscoreWindow()
    {
        windowStackPushWindow_(WindowKind_LocalHiscore);
    }
    inline void windowStackPushOilStatusWindow() { windowStackPushWindow_(WindowKind_OilStatus); }
    inline void windowStackPushShopWindow() { windowStackPushWindow_(WindowKind_Shop); }
    inline void windowStackPushKeypadWindow() { windowStackPushWindow_(WindowKind_Keypad); }
    inline void windowStackPushAudioCacheProgressWindow()
    {
        windowStackPushWindow_(WindowKind_AudioCacheProgress);
    }
    inline void windowStackPushNewGameWindow() { windowStackPushWindow_(WindowKind_NewGame); }

    // Generic text entry (Keypad) helper.
    // - `title` should outlive the keypad session (string literal is perfect).
    // - `outText/outLen` are owned by caller; keypad writes back into them on Enter.
    inline void windowStackPushKeypadEditor(
        Keypad *keypad,
        const char *title,
        char *outText,
        int32_t *outLen
    )
    {
        initKeypad(keypad, outText, outLen);
        keypad->title = title;
        keypad->activated = true;
        uploadKeypadText(keypad);
        windowStackPushWindow_(WindowKind_Keypad);
    }

    // Returns true if event is consumed by the active (topmost) window and must not reach the game.
    bool processActiveWindowEvent(
        Clayton *clayton,
        Keypad *keypad,
        Storage *storage,
        SoundSettings *soundSettings,
        AdaptiveAudioSystem *adaptiveAudio,
        LocalHighscore *localHi,
        CarouselState *carousel,
        bool *shouldShowShop,
        SDL_Event e
    );

    // Renders bottom -> top, drawing dim overlays between windows (topmost not dimmed).
    void renderWindowStack(
        Clayton *clayton,
        Keypad *keypad,
        SoundSettings *soundSettings,
        AdaptiveAudioSystem *adaptiveAudio,
        LocalHighscore *localHi,
        CarouselState *carousel,
        bool shouldShowShop
    );

private:
    inline bool windowStackHasTopWindow_() const { return count > 0; }

    inline WindowKind windowStackTopWindowKind_() const
    {
        // Caller should check windowStackHasTopWindow_(); this default keeps UB away during refactors.
        return (count > 0) ? kinds[count - 1] : WindowKind_Keypad;
    }

    inline void windowStackPopTopWindow_()
    {
        if (count > 0)
        {
            count--;
        }
        // Any time the stack changes, clear one-shot requests that depend on a specific window
        // still being active. (Do NOT clear playAgainRequested here; it's a one-shot signal from
        // the window to game.cpp, and we often pop the window in the same event that sets it.)
        shopBuyRequested = false;
    }

    inline void windowStackRemoveWindow_(WindowKind kind)
    {
        // Stable remove (preserves relative order of remaining windows).
        for (int i = 0; i < count; i++)
        {
            if (kinds[i] == kind)
            {
                for (int j = i + 1; j < count; j++)
                {
                    kinds[j - 1] = kinds[j];
                }
                count--;
                return;
            }
        }
    }

    inline void windowStackPushWindow_(WindowKind kind)
    {
        // Dedupe: if present, remove first so we can move-to-top.
        windowStackRemoveWindow_(kind);
        // Any time the stack changes, clear one-shot requests that depend on a specific window
        // still being active.
        shopBuyRequested = false;

        if (count >= WINDOW_STACK_MAX)
        {
            // If you hit this, increase WINDOW_STACK_MAX or decide eviction policy.
            // Default: refuse to push.
            return;
        }
        kinds[count++] = kind;
    }

    // ---- Internal dispatch helpers (explicit and searchable) ----
    static bool processAdaptiveAudioWindowEvent(
        WindowStack *self,
        Clayton *clayton,
        AdaptiveAudioSystem *adaptiveAudio,
        SDL_Event e
    );
    static bool processSoundSettingsWindowEvent(
        WindowStack *self,
        Clayton *clayton,
        SoundSettings *soundSettings,
        SDL_Event e
    );
    static bool processLocalHiscoreWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e);
    static bool processOilStatusWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e);
    static bool processShopWindowEvent(
        WindowStack *self,
        Clayton *clayton,
        CarouselState *carousel,
        bool *shouldShowShop,
        SDL_Event e
    );
    static bool processKeypadWindowEvent(
        WindowStack *self,
        Keypad *keypad,
        Storage *storage,
        SDL_Event e
    );
    static bool processAudioCacheProgressWindowEvent(WindowStack *self, SDL_Event e);
    static bool processNewGameWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e);

    static void renderAdaptiveAudioWindow(Clayton *clayton, AdaptiveAudioSystem *adaptiveAudio);
    static void renderSoundSettingsWindow(Clayton *clayton, SoundSettings *soundSettings);
    static void renderLocalHiscoreWindow(Clayton *clayton, LocalHighscore *localHi);
    static void renderOilStatusWindow(Clayton *clayton, CarouselState *carousel);
    static void renderShopWindow(Clayton *clayton, CarouselState *carousel);
    static void renderKeypadWindow(Keypad *keypad);
    static void renderAudioCacheProgressWindow(Clayton *clayton);
    static void renderNewGameWindow(Clayton *clayton);
};

// ----------------------------------------------------------------------------
// Implementation (header-only module)
// ----------------------------------------------------------------------------

inline bool WindowStack::processActiveWindowEvent(
    Clayton *clayton,
    Keypad *keypad,
    Storage *storage,
    SoundSettings *soundSettings,
    AdaptiveAudioSystem *adaptiveAudio,
    LocalHighscore * /*localHi*/,
    CarouselState *carousel,
    bool *shouldShowShop,
    SDL_Event e
)
{
    if (!windowStackHasTopWindow_())
    {
        return false;
    }

    const WindowKind top = windowStackTopWindowKind_();
    bool consumed = false;

    switch (top)
    {
    case WindowKind_Keypad:
        consumed = processKeypadWindowEvent(this, keypad, storage, e);
        if (keypad && !keypad->activated)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_SoundSettings:
        consumed = processSoundSettingsWindowEvent(this, clayton, soundSettings, e);
        if (soundSettings && !soundSettings->activated)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_LocalHiscore:
        consumed = processLocalHiscoreWindowEvent(this, clayton, e);
        if (clayton && !clayton->shouldShowHiScore)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_OilStatus:
        consumed = processOilStatusWindowEvent(this, clayton, e);
        if (clayton && !clayton->shouldShowOilStatus)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_Shop:
        consumed = processShopWindowEvent(this, clayton, carousel, shouldShowShop, e);
        if (shouldShowShop && !*shouldShowShop)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_AdaptiveAudio:
        consumed = processAdaptiveAudioWindowEvent(this, clayton, adaptiveAudio, e);
        if (adaptiveAudio && !(adaptiveAudio->showModal || adaptiveAudio->state == ADAPTIVE_EXPORTING ||
                               adaptiveAudio->state == ADAPTIVE_DECIDING))
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_AudioCacheProgress:
        consumed = processAudioCacheProgressWindowEvent(this, e);
        // Removal is driven by the caller: push this window only while something is exporting.
        // If top is cache-progress but nothing is actually running, auto-pop.
        if (!(adaptiveAudio && adaptiveAudio->state == ADAPTIVE_EXPORTING) &&
            !(soundSettings && soundSettings->wavExportInProgress))
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_NewGame:
        consumed = processNewGameWindowEvent(this, clayton, e);
        return consumed;
    }

    return false;
}

inline void WindowStack::renderWindowStack(
    Clayton *clayton,
    Keypad *keypad,
    SoundSettings *soundSettings,
    AdaptiveAudioSystem *adaptiveAudio,
    LocalHighscore *localHi,
    CarouselState *carousel,
    bool shouldShowShop
)
{
    if (count <= 0)
    {
        return;
    }

    // Overlay should cover the entire screen, but windows should be constrained to the portrait
    // middle column ("Portrait area"). We compute that bounding box and render windows into a
    // floating viewport aligned to that region.
    const int topIdx = count - 1;
    const int16_t baseZ = 100;

    Clay_BoundingBox rootBox = Clay_GetElementData(CLAY_ID("Root")).boundingBox;
    Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
    const float rootCx = rootBox.x + rootBox.width * 0.5f;
    const float rootCy = rootBox.y + rootBox.height * 0.5f;
    const float portraitCx = portraitBox.x + portraitBox.width * 0.5f;
    const float portraitCy = portraitBox.y + portraitBox.height * 0.5f;
    const Clay_Vector2 portraitOffset = {portraitCx - rootCx, portraitCy - rootCy};

    // 1) Bottom..(top-1) windows
    // 2) Single dim overlay before topmost window (dims everything below, including the game/HUD).
    // This also applies when there is only one window: we still want the background dimmed but not
    // the window itself.
    {
        int overlayZCalc = (int)(baseZ + topIdx * 2 - 1);
        if (overlayZCalc < 0)
            overlayZCalc = 0;
        const int16_t overlayZ = (int16_t)overlayZCalc;
        CLAY(
            CLAY_ID("WindowStackDimOverlay"),
            {
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                    },
                .backgroundColor = CLAY_COLOR_WINDOW_STACK_OVERLAY,
                .floating = {
                    .offset = {0},
                    .zIndex = overlayZ,
                    .attachPoints =
                        {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }
        )
        {
        }
    }

    // 3) Window viewport aligned to portrait area (middle column). Windows float inside this.
    {
        CLAY(
            CLAY_ID("WindowStackViewport"),
            {
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_FIXED(portraitBox.width),
                                   .height = CLAY_SIZING_FIXED(portraitBox.height)},
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                    },
                .floating = {
                    .offset = portraitOffset,
                    .zIndex = baseZ,
                    .attachPoints =
                        {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }
        )
        {
            // Bottom..(top-1) windows
            for (int i = 0; i < topIdx; i++)
            {
                const int16_t z = (int16_t)(baseZ + i * 2);
                CLAY(
                    CLAY_IDI("WindowStackWindow", i),
                    {
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                            },
                        .floating = {
                            .offset = {0},
                            .zIndex = z,
                            .attachPoints =
                                {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                        },
                    }
                )
                {
                    switch (kinds[i])
                    {
                    case WindowKind_Keypad:
                        if (keypad && keypad->activated)
                            renderKeypadWindow(keypad);
                        break;
                    case WindowKind_SoundSettings:
                        if (soundSettings && soundSettings->activated &&
                            !soundSettings->wavExportInProgress)
                            renderSoundSettingsWindow(clayton, soundSettings);
                        break;
                    case WindowKind_LocalHiscore:
                        if (clayton && clayton->shouldShowHiScore && !shouldShowShop)
                            renderLocalHiscoreWindow(clayton, localHi);
                        break;
                    case WindowKind_OilStatus:
                        if (clayton && clayton->shouldShowOilStatus && !shouldShowShop)
                            renderOilStatusWindow(clayton, carousel);
                        break;
                    case WindowKind_Shop:
                        if (shouldShowShop)
                            renderShopWindow(clayton, carousel);
                        break;
                    case WindowKind_AdaptiveAudio:
                        if (adaptiveAudio &&
                            (adaptiveAudio->showModal || adaptiveAudio->state == ADAPTIVE_EXPORTING ||
                             adaptiveAudio->state == ADAPTIVE_DECIDING))
                            renderAdaptiveAudioWindow(clayton, adaptiveAudio);
                        break;
                    case WindowKind_AudioCacheProgress:
                        renderAudioCacheProgressWindow(clayton);
                        break;
                    case WindowKind_NewGame:
                        renderNewGameWindow(clayton);
                        break;
                    }
                }
            }

            // Topmost window
            {
                const int i = topIdx;
                const int16_t z = (int16_t)(baseZ + topIdx * 2);
                CLAY(
                    CLAY_IDI("WindowStackTopWindow", i),
                    {
                        .layout =
                            {
                                .sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                            },
                        .floating = {
                            .offset = {0},
                            .zIndex = z,
                            .attachPoints =
                                {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                        },
                    }
                )
                {
                    switch (kinds[i])
                    {
                    case WindowKind_Keypad:
                        if (keypad && keypad->activated)
                            renderKeypadWindow(keypad);
                        break;
                    case WindowKind_SoundSettings:
                        if (soundSettings && soundSettings->activated &&
                            !soundSettings->wavExportInProgress)
                            renderSoundSettingsWindow(clayton, soundSettings);
                        break;
                    case WindowKind_LocalHiscore:
                        if (clayton && clayton->shouldShowHiScore && !shouldShowShop)
                            renderLocalHiscoreWindow(clayton, localHi);
                        break;
                    case WindowKind_OilStatus:
                        if (clayton && clayton->shouldShowOilStatus && !shouldShowShop)
                            renderOilStatusWindow(clayton, carousel);
                        break;
                    case WindowKind_Shop:
                        if (shouldShowShop)
                            renderShopWindow(clayton, carousel);
                        break;
                    case WindowKind_AdaptiveAudio:
                        if (adaptiveAudio &&
                            (adaptiveAudio->showModal || adaptiveAudio->state == ADAPTIVE_EXPORTING ||
                             adaptiveAudio->state == ADAPTIVE_DECIDING))
                            renderAdaptiveAudioWindow(clayton, adaptiveAudio);
                        break;
                    case WindowKind_AudioCacheProgress:
                        renderAudioCacheProgressWindow(clayton);
                        break;
                    case WindowKind_NewGame:
                        renderNewGameWindow(clayton);
                        break;
                    }
                }
            }
        }
    }
}

// ---- Internal dispatch helpers ----

inline bool WindowStack::processKeypadWindowEvent(
    WindowStack * /*self*/,
    Keypad *keypad,
    Storage *storage,
    SDL_Event e
)
{
    if (!keypad || !keypad->activated)
    {
        return false;
    }
    return processKeypadEvent(keypad, e, storage);
}

inline bool WindowStack::processSoundSettingsWindowEvent(
    WindowStack * /*self*/,
    Clayton *clayton,
    SoundSettings *soundSettings,
    SDL_Event e
)
{
    if (!soundSettings || !soundSettings->activated)
    {
        return false;
    }
    return processSoundSettingsEvent(clayton, soundSettings, e);
}

inline bool WindowStack::processLocalHiscoreWindowEvent(
    WindowStack * /*self*/,
    Clayton *clayton,
    SDL_Event e
)
{
    if (!clayton || !clayton->shouldShowHiScore)
    {
        return false;
    }

    // Close button
    if (isClaytonClicked(&clayton->hiScoreCloseClick, e))
    {
        clayton->shouldShowHiScore = false;
        clayton->shouldShowHiScoreWithLatest = false;
        return true;
    }

    // Consume clicks over the overlay to prevent click-through.
    if (Clay_PointerOver(CLAY_ID("HiScoreContainer")))
    {
        const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
        const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
        const bool mouseMove = e.type == SDL_MOUSEMOTION;
        if (mouseDown || mouseUp || mouseMove)
        {
            return true;
        }
    }

    return false;
}

inline bool WindowStack::processOilStatusWindowEvent(
    WindowStack *self,
    Clayton *clayton,
    SDL_Event e
)
{
    if (!clayton || !clayton->shouldShowOilStatus)
    {
        return false;
    }

    if (isClaytonClicked(&clayton->oilStatusCloseClick, e))
    {
        clayton->shouldShowOilStatus = false;
        return true;
    }

    if (self && isClaytonClicked(&clayton->oilReoilClick, e))
    {
        self->oilReoilRequested = true;
        return true;
    }

    if (Clay_PointerOver(CLAY_ID("OilStatusContainer")))
    {
        const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
        const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
        const bool mouseMove = e.type == SDL_MOUSEMOTION;
        if (mouseDown || mouseUp || mouseMove)
        {
            return true;
        }
    }

    return false;
}

inline bool WindowStack::processShopWindowEvent(
    WindowStack *self,
    Clayton *clayton,
    CarouselState *carousel,
    bool *shouldShowShop,
    SDL_Event e
)
{
    if (!shouldShowShop || !*shouldShowShop)
    {
        return false;
    }

    // Close / buy buttons are Clay clicks.
    if (isClaytonClicked(&clayton->closeShopClick, e))
    {
        *shouldShowShop = false;
        self->shopPointerDown = false;
        return true;
    }
    if (isClaytonClicked(&clayton->buyClick, e))
    {
        // Purchase logic lives in game.cpp (needs UserContext).
        // We set a one-shot request flag and consume the click so nothing else sees it.
        self->shopBuyRequested = true;
        return true;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        if (carousel)
        {
            // TODO: thread a "now time" value into processActiveWindowEvent so this matches game.cpp.
            Carousel_OnPointerDown(carousel, e.button.x, e.button.y, 0.0f);
        }
        self->shopPointerDown = true;
        self->shopLastX = e.button.x;
        self->shopLastY = e.button.y;
        return true;
    }
    if (e.type == SDL_MOUSEMOTION)
    {
        if (carousel && self->shopPointerDown)
        {
            const int dx = e.motion.x - self->shopLastX;
            const int dy = e.motion.y - self->shopLastY;
            self->shopLastX = e.motion.x;
            self->shopLastY = e.motion.y;
            Carousel_OnPointerMove(carousel, dx, dy);
        }
        return true;
    }
    if (e.type == SDL_MOUSEBUTTONUP)
    {
        if (carousel)
        {
            Carousel_OnPointerUp(carousel, e.button.x, e.button.y, 0.0f);
        }
        self->shopPointerDown = false;
        return true;
    }

    // Consume pointer events over the overlay even if we didn't match anything.
    if (Clay_PointerOver(CLAY_ID("ShopOverlay")))
    {
        const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
        const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
        const bool mouseMove = e.type == SDL_MOUSEMOTION;
        if (mouseDown || mouseUp || mouseMove)
        {
            return true;
        }
    }

    return false;
}

inline bool WindowStack::processAdaptiveAudioWindowEvent(
    WindowStack * /*self*/,
    Clayton *clayton,
    AdaptiveAudioSystem *adaptiveAudio,
    SDL_Event e
)
{
    if (!adaptiveAudio)
    {
        return false;
    }
    // AdaptiveAudio_ProcessEvent2 consumes events for the "DECIDING" modal (button clicks).
    // During "EXPORTING" we still need to block click-through into the game.
    if (AdaptiveAudio_ProcessEvent2(clayton, adaptiveAudio, e))
    {
        return true;
    }

    if (adaptiveAudio->state == ADAPTIVE_EXPORTING)
    {
        const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
        const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
        const bool mouseMove = e.type == SDL_MOUSEMOTION;
        const bool mouseWheel = e.type == SDL_MOUSEWHEEL;

        if (mouseDown || mouseUp || mouseMove || mouseWheel)
        {
            // Consume pointer events while the exporting overlay is visible.
            if (Clay_PointerOver(CLAY_ID("AdaptiveOverlay")) ||
                Clay_PointerOver(CLAY_ID("AdaptiveAudioContainer")))
            {
                return true;
            }
        }
    }

    return false;
}

inline bool WindowStack::processAudioCacheProgressWindowEvent(WindowStack * /*self*/, SDL_Event e)
{
    // Default: consume pointer events so the game doesn't click through.
    const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
    const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
    const bool mouseMove = e.type == SDL_MOUSEMOTION;
    return mouseDown || mouseUp || mouseMove;
}

inline bool WindowStack::processNewGameWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e)
{
    if (!self || !clayton)
    {
        return false;
    }

    const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_FINGERDOWN;
    const bool mouseUp = e.type == SDL_MOUSEBUTTONUP || e.type == SDL_FINGERUP;
    if (!mouseDown && !mouseUp)
    {
        return false;
    }

    if (isClaytonClicked(&clayton->playAgainClick, e))
    {
        self->playAgainRequested = true;
        // NewGame is a modal action window; after clicking, remove it immediately.
        self->windowStackPopTopWindow_();
        return true;
    }

    // If pointer is over the window, consume the event (even if not on the button).
    if (Clay_PointerOver(CLAY_ID("NewGameWindowContainer")))
    {
        return true;
    }

    return false;
}

// ---- Render helpers ----

inline void WindowStack::renderKeypadWindow(Keypad *keypad) { buildKeypadWindowClay(keypad); }

inline void WindowStack::renderSoundSettingsWindow(Clayton *clayton, SoundSettings *soundSettings)
{
    buildSoundSettingsWindowClay(clayton, soundSettings);
}

inline void WindowStack::renderLocalHiscoreWindow(Clayton *clayton, LocalHighscore *localHi)
{
    buildHiScoreWindowClay(clayton, localHi);
}

inline void WindowStack::renderOilStatusWindow(Clayton *clayton, CarouselState *carousel)
{
    buildOilStatusWindowClay(clayton, carousel ? carousel->bank : 0.0f);
}

inline void WindowStack::renderShopWindow(Clayton *clayton, CarouselState *carousel)
{
    RenderShopWindow_Carousel(clayton, carousel, (carousel ? carousel->bank : 0.0f), "Cauntdaun");
}

inline void WindowStack::renderAdaptiveAudioWindow(Clayton *clayton, AdaptiveAudioSystem *adaptiveAudio)
{
    AdaptiveAudio_RenderWindowUI(clayton, adaptiveAudio);
}

inline void WindowStack::renderAudioCacheProgressWindow(Clayton * /*clayton*/)
{
    // Placeholder: rendering the cache-progress indicator should be unified later.
    // (Today adaptive export + wav export already render their own UIs.)
}

inline void WindowStack::renderNewGameWindow(Clayton *clayton)
{
    ::renderNewGameWindow(clayton);
}
