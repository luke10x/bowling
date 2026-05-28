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
#include <string.h>

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
#include "../oil/oil_status.h"
#include "../oil/oil_status.h"
#include "../oil/oil_clay.h"
#include "../houses/houses_clay.h"
#include "../houses/houses.h"
#include "../bots/bots_clay.h"
#include "../bots/bots.h"
#include "../tracker/tracker_clay.h"
#include "../clayton/slider.h"
#include "menu_clay.h"
// Keep this small; we statically allocate in WindowStack.
#ifndef WINDOW_STACK_MAX
#define WINDOW_STACK_MAX 10
#endif

enum WindowKind // I like it 
{
    WindowKind_Menu,
    WindowKind_AdaptiveAudio,
    WindowKind_SoundSettings,
    WindowKind_LocalHiscore,
    WindowKind_OilStatus,
    WindowKind_Houses,
    WindowKind_BotSelect,
    WindowKind_Shop,
    WindowKind_Keypad,
    WindowKind_AudioCacheProgress,
    WindowKind_NewGame,
    WindowKind_MassEditor,
    WindowKind_BotResult,
    WindowKind_TrackerEditor,
    WindowKind_TrackerInstruments,
    WindowKind_TrackerSongSettings,
    WindowKind_TrackerInstrumentEditor,
    WindowKind_TrackerInstrumentColor,
    WindowKind_TrackerOperatorEditor,
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
    bool housesPointerDown;
    int housesLastX;
    int housesLastY;
    bool housesSelectRequested;
    bool botPointerDown;
    int botLastX;
    int botLastY;
    bool menuRenameRequested;
    bool menuSchoolRequested;
    bool menuTrackerRequested;
    bool menuTrackerVisible;
    bool botSelectRequested;
    int botSelectedKind;

    // BOT result modal (shown after BOT games, before hiscore window).
    int botResultPlayerScore;
    int botResultAngelScore;
    bool botResultPlayerWon;

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
        housesPointerDown = false;
        housesLastX = 0;
        housesLastY = 0;
        housesSelectRequested = false;
        botPointerDown = false;
        botLastX = 0;
        botLastY = 0;
        menuRenameRequested = false;
        menuSchoolRequested = false;
        menuTrackerRequested = false;
        menuTrackerVisible = true;
        botSelectRequested = false;
        botSelectedKind = 0;
        botResultPlayerScore = 0;
        botResultAngelScore = 0;
        botResultPlayerWon = false;
    }

    // ---- Push helpers (call sites never mention WindowKind) ----
    inline void windowStackPushAdaptiveAudioWindow()
    {
        windowStackPushWindow_(WindowKind_AdaptiveAudio);
    }
    inline void windowStackPushMenuWindow() { windowStackPushWindow_(WindowKind_Menu); }
    inline void windowStackPushSoundSettingsWindow()
    {
        windowStackPushWindow_(WindowKind_SoundSettings);
    }
    inline void windowStackPushLocalHiscoreWindow()
    {
        windowStackPushWindow_(WindowKind_LocalHiscore);
    }
    inline void windowStackPushOilStatusWindow() { windowStackPushWindow_(WindowKind_OilStatus); }
    inline void windowStackPushHousesWindow() { windowStackPushWindow_(WindowKind_Houses); }
    inline void windowStackPushBotSelectWindow() { windowStackPushWindow_(WindowKind_BotSelect); }
    inline void windowStackPushShopWindow() { windowStackPushWindow_(WindowKind_Shop); }
    inline void windowStackPushKeypadWindow() { windowStackPushWindow_(WindowKind_Keypad); }
    inline void windowStackPushAudioCacheProgressWindow()
    {
        windowStackPushWindow_(WindowKind_AudioCacheProgress);
    }
    inline void windowStackPushNewGameWindow() { windowStackPushWindow_(WindowKind_NewGame); }
    inline void windowStackPushMassEditorWindow() { windowStackPushWindow_(WindowKind_MassEditor); }
    inline void windowStackPushTrackerEditorWindow() { windowStackPushWindow_(WindowKind_TrackerEditor); }
    inline void windowStackPushTrackerInstrumentsWindow() { windowStackPushWindow_(WindowKind_TrackerInstruments); }
    inline void windowStackPushTrackerSongSettingsWindow() { windowStackPushWindow_(WindowKind_TrackerSongSettings); }
    inline void windowStackPushTrackerInstrumentEditorWindow() { windowStackPushWindow_(WindowKind_TrackerInstrumentEditor); }
    inline void windowStackPushTrackerInstrumentColorWindow() { windowStackPushWindow_(WindowKind_TrackerInstrumentColor); }
    inline void windowStackPushTrackerOperatorEditorWindow() { windowStackPushWindow_(WindowKind_TrackerOperatorEditor); }
    inline void windowStackPushBotResultWindow(int playerScore, int angelScore, bool playerWon)
    {
        botResultPlayerScore = playerScore;
        botResultAngelScore = angelScore;
        botResultPlayerWon = playerWon;
        windowStackPushWindow_(WindowKind_BotResult);
    }

    // Immediate close helper (use sparingly). Most code should close via `clayton->shouldShowX = false`
    // and let event processing pop the window, but some flows need to close a window and show a dialog
    // in the same tick.
    inline void windowStackCloseTopWindow() { windowStackPopTopWindow_(); }
    // Generic text entry (Keypad) helper.
    // - `title` should outlive the keypad session (string literal is perfect).
    // - `outText/outLen` are owned by caller; keypad writes back into them on Enter.
    inline void windowStackPushKeypadEditor(
        Keypad *keypad,
        const char *title,
        char *outText,
        int32_t *outLen,
        bool persistUsernameToStorage = true
    )
    {
        initKeypad(keypad, outText, outLen);
        keypad->title = title;
        keypad->persistUsernameToStorage = persistUsernameToStorage;
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
        HouseCarouselState *houses,
        BotCarouselState *bots,
        Tracker *tracker,
        Clayton_Slider *massSlider,
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
        HouseCarouselState *houses,
        BotCarouselState *bots,
        Tracker *tracker,
        Clayton_Slider *massSlider,
        bool shouldShowShop,
        bool showTrackerInMenu,
        const OilStatusUI *oilStatus,
        float deltaTime
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
    static bool processHousesWindowEvent(WindowStack *self, Clayton *clayton, HouseCarouselState *houses, SDL_Event e);
    static bool processBotSelectWindowEvent(WindowStack *self, Clayton *clayton, BotCarouselState *bots, SDL_Event e);
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
    static bool processMenuWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e);
    static bool processMassEditorWindowEvent(WindowStack *self, Clayton *clayton, Clayton_Slider *massSlider, SDL_Event e);
    static bool processBotResultWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e);
    static bool processTrackerEditorWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static bool processTrackerInstrumentsWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static bool processTrackerSongSettingsWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static bool processTrackerInstrumentEditorWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static bool processTrackerInstrumentColorWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static bool processTrackerOperatorEditorWindowEvent(WindowStack *self, Tracker *tracker, SDL_Event e);
    static void renderAdaptiveAudioWindow(Clayton *clayton, AdaptiveAudioSystem *adaptiveAudio);
    static void renderSoundSettingsWindow(Clayton *clayton, SoundSettings *soundSettings);
    static void renderLocalHiscoreWindow(Clayton *clayton, LocalHighscore *localHi);
    static void renderOilStatusWindow(Clayton *clayton, CarouselState *carousel, const OilStatusUI *oilStatus);
    static void renderHousesWindow(Clayton *clayton, HouseCarouselState *houses, float deltaTime);
    static void renderBotSelectWindow(Clayton *clayton, BotCarouselState *bots, float deltaTime);
    static void renderShopWindow(Clayton *clayton, CarouselState *carousel);
    static void renderKeypadWindow(Keypad *keypad);
    static void renderAudioCacheProgressWindow(Clayton *clayton);
    static void renderNewGameWindow(Clayton *clayton);
    static void renderMenuWindow(Clayton *clayton, bool showGoToSchool, bool showTracker);
    static void renderMassEditorWindow(Clayton *clayton, Clayton_Slider *massSlider);
    static void renderBotResultWindow(WindowStack *self, Clayton *clayton);
    static void renderTrackerEditorWindow(Clayton *clayton, Tracker *tracker);
    static void renderTrackerInstrumentsWindow(Clayton *clayton, Tracker *tracker);
    static void renderTrackerSongSettingsWindow(Clayton *clayton, Tracker *tracker);
    static void renderTrackerInstrumentEditorWindow(Clayton *clayton, Tracker *tracker);
    static void renderTrackerInstrumentColorWindow(Clayton *clayton, Tracker *tracker);
    static void renderTrackerOperatorEditorWindow(Clayton *clayton, Tracker *tracker);
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
    HouseCarouselState *houses,
    BotCarouselState *bots,
    Tracker *tracker,
    Clayton_Slider *massSlider,
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
    case WindowKind_Menu:
        consumed = processMenuWindowEvent(this, clayton, e);
        return consumed;

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

    case WindowKind_Houses:
        consumed = processHousesWindowEvent(this, clayton, houses, e);
        if (clayton && !clayton->shouldShowHouses)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_BotSelect:
        consumed = processBotSelectWindowEvent(this, clayton, bots, e);
        if (clayton && !clayton->shouldShowBotSelect)
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

    case WindowKind_MassEditor:
        consumed = processMassEditorWindowEvent(this, clayton, massSlider, e);
        return consumed;

    case WindowKind_BotResult:
        consumed = processBotResultWindowEvent(this, clayton, e);
        return consumed;

    case WindowKind_TrackerEditor:
        consumed = processTrackerEditorWindowEvent(this, tracker, e);
        if (tracker && !tracker->editorOpen)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_TrackerInstruments:
        consumed = processTrackerInstrumentsWindowEvent(this, tracker, e);
        if (tracker && !tracker->instrumentsWindowOpen)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_TrackerSongSettings:
        consumed = processTrackerSongSettingsWindowEvent(this, tracker, e);
        if (tracker && !tracker->songSettingsWindowOpen)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_TrackerInstrumentEditor:
        consumed = processTrackerInstrumentEditorWindowEvent(this, tracker, e);
        if (tracker && !tracker->instrumentEditorOpen)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_TrackerInstrumentColor:
        consumed = processTrackerInstrumentColorWindowEvent(this, tracker, e);
        if (tracker && !tracker->instrumentColorWindowOpen)
        {
            windowStackPopTopWindow_();
        }
        return consumed;

    case WindowKind_TrackerOperatorEditor:
        consumed = processTrackerOperatorEditorWindowEvent(this, tracker, e);
        if (tracker && !tracker->operatorEditorOpen)
        {
            windowStackPopTopWindow_();
        }
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
    HouseCarouselState *houses,
    BotCarouselState *bots,
    Tracker *tracker,
    Clayton_Slider *massSlider,
    bool shouldShowShop,
    bool showTrackerInMenu,
    const OilStatusUI *oilStatus,
    float deltaTime
)
{
    menuTrackerVisible = showTrackerInMenu;
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
                    case WindowKind_Menu:
                        renderMenuWindow(clayton, true, menuTrackerVisible);
                        break;
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
                            renderOilStatusWindow(clayton, carousel, oilStatus);
                        break;
                    case WindowKind_Houses:
                        if (clayton && clayton->shouldShowHouses && !shouldShowShop)
                            renderHousesWindow(clayton, houses, deltaTime);
                        break;
                    case WindowKind_BotSelect:
                        if (clayton && clayton->shouldShowBotSelect && !shouldShowShop)
                            renderBotSelectWindow(clayton, bots, deltaTime);
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
                    case WindowKind_MassEditor:
                        renderMassEditorWindow(clayton, massSlider);
                        break;
                    case WindowKind_BotResult:
                        renderBotResultWindow(this, clayton);
                        break;
                    case WindowKind_TrackerEditor:
                        renderTrackerEditorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstruments:
                        renderTrackerInstrumentsWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerSongSettings:
                        renderTrackerSongSettingsWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstrumentEditor:
                        renderTrackerInstrumentEditorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstrumentColor:
                        renderTrackerInstrumentColorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerOperatorEditor:
                        renderTrackerOperatorEditorWindow(clayton, tracker);
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
                    case WindowKind_Menu:
                        renderMenuWindow(clayton, true, menuTrackerVisible);
                        break;
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
                            renderOilStatusWindow(clayton, carousel, oilStatus);
                        break;
                    case WindowKind_Houses:
                        if (clayton && clayton->shouldShowHouses && !shouldShowShop)
                            renderHousesWindow(clayton, houses, deltaTime);
                        break;
                    case WindowKind_BotSelect:
                        if (clayton && clayton->shouldShowBotSelect && !shouldShowShop)
                            renderBotSelectWindow(clayton, bots, deltaTime);
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
                    case WindowKind_MassEditor:
                        renderMassEditorWindow(clayton, massSlider);
                        break;
                    case WindowKind_BotResult:
                        renderBotResultWindow(this, clayton);
                        break;
                    case WindowKind_TrackerEditor:
                        renderTrackerEditorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstruments:
                        renderTrackerInstrumentsWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerSongSettings:
                        renderTrackerSongSettingsWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstrumentEditor:
                        renderTrackerInstrumentEditorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerInstrumentColor:
                        renderTrackerInstrumentColorWindow(clayton, tracker);
                        break;
                    case WindowKind_TrackerOperatorEditor:
                        renderTrackerOperatorEditorWindow(clayton, tracker);
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

inline bool WindowStack::processBotResultWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e)
{
    if (!self || !clayton)
        return false;

    if (isClaytonClicked(&clayton->botResultCloseClick, e))
    {
        self->windowStackPopTopWindow_();
        return true;
    }

    const bool isPointerEvent =
        (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_MOUSEBUTTONUP) ||
        (e.type == SDL_MOUSEMOTION) || (e.type == SDL_MOUSEWHEEL) ||
        (e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION);

    // Consume pointer events while modal is visible to prevent click-through.
    return isPointerEvent;
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

inline bool WindowStack::processHousesWindowEvent(
    WindowStack *self,
    Clayton *clayton,
    HouseCarouselState *houses,
    SDL_Event e
)
{
    if (!clayton || !clayton->shouldShowHouses)
    {
        return false;
    }

    if (isClaytonClicked(&clayton->housesCloseClick, e))
    {
        clayton->shouldShowHouses = false;
        return true;
    }

    if (self && isClaytonClicked(&clayton->housesSelectClick, e))
    {
        self->housesSelectRequested = true;
        // Houses is a modal selection window; after selecting, close it immediately.
        clayton->shouldShowHouses = false;
        self->windowStackPopTopWindow_();
        return true;
    }

    // Carousel drag
    if (houses)
    {
        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            HousesCarousel_OnPointerDown(houses, e.button.x);
            self->housesPointerDown = true;
            self->housesLastX = e.button.x;
            self->housesLastY = e.button.y;
            return Clay_PointerOver(CLAY_ID("HousesContainer"));
        }
        if (e.type == SDL_MOUSEMOTION)
        {
            if (self->housesPointerDown)
            {
                const int dx = e.motion.x - self->housesLastX;
                self->housesLastX = e.motion.x;
                self->housesLastY = e.motion.y;
                HousesCarousel_OnPointerMove(houses, (float)dx);
            }
            return Clay_PointerOver(CLAY_ID("HousesContainer"));
        }
        if (e.type == SDL_MOUSEBUTTONUP)
        {
            HousesCarousel_OnPointerUp(houses);
            self->housesPointerDown = false;
            return Clay_PointerOver(CLAY_ID("HousesContainer"));
        }
    }

    if (Clay_PointerOver(CLAY_ID("HousesContainer")))
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

inline bool WindowStack::processBotSelectWindowEvent(
    WindowStack *self,
    Clayton *clayton,
    BotCarouselState *bots,
    SDL_Event e
)
{
    if (!clayton || !clayton->shouldShowBotSelect)
    {
        return false;
    }

    if (isClaytonClicked(&clayton->botSelectCloseClick, e))
    {
        clayton->shouldShowBotSelect = false;
        return true;
    }

    if (self && isClaytonClicked(&clayton->botSelectSelectClick, e))
    {
        const int idx = bots ? bots->closestBotIdx : -1;
        if (bots && idx >= 0 && idx < bots->cardCount)
        {
            self->botSelectedKind = (int)bots->items[idx].kind;
            self->botSelectRequested = true;
        }

        // Modal selection: close immediately (returns to menu unless caller clears the stack).
        clayton->shouldShowBotSelect = false;
        self->windowStackPopTopWindow_();
        return true;
    }

    // Carousel drag
    if (self && bots)
    {
        if (e.type == SDL_MOUSEBUTTONDOWN)
        {
            BotsCarousel_OnPointerDown(bots, e.button.x);
            self->botPointerDown = true;
            self->botLastX = e.button.x;
            self->botLastY = e.button.y;
            return Clay_PointerOver(CLAY_ID("BotsContainer"));
        }
        if (e.type == SDL_MOUSEMOTION)
        {
            if (self->botPointerDown)
            {
                const int dx = e.motion.x - self->botLastX;
                self->botLastX = e.motion.x;
                self->botLastY = e.motion.y;
                BotsCarousel_OnPointerMove(bots, (float)dx);
            }
            return Clay_PointerOver(CLAY_ID("BotsContainer"));
        }
        if (e.type == SDL_MOUSEBUTTONUP)
        {
            BotsCarousel_OnPointerUp(bots);
            self->botPointerDown = false;
            return Clay_PointerOver(CLAY_ID("BotsContainer"));
        }
    }

    if (Clay_PointerOver(CLAY_ID("BotsContainer")))
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

inline bool WindowStack::processMenuWindowEvent(WindowStack *self, Clayton *clayton, SDL_Event e)
{
    if (!self || !clayton)
        return false;

    const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_FINGERDOWN;
    const bool mouseUp = e.type == SDL_MOUSEBUTTONUP || e.type == SDL_FINGERUP;
    const bool mouseMove = e.type == SDL_MOUSEMOTION || e.type == SDL_FINGERMOTION;
    if (!mouseDown && !mouseUp && !mouseMove)
        return false;

    if (isClaytonClicked(&clayton->menuCloseClick, e))
    {
        self->windowStackPopTopWindow_();
        return true;
    }
    if (isClaytonClicked(&clayton->menuRenameClick, e))
    {
        self->menuRenameRequested = true;
        self->windowStackPopTopWindow_();
        return true;
    }
    if (isClaytonClicked(&clayton->menuSchoolClick, e))
    {
        self->menuSchoolRequested = true;
        self->windowStackPopTopWindow_();
        return true;
    }
    if (self->menuTrackerVisible && isClaytonClicked(&clayton->menuTrackerClick, e))
    {
        self->menuTrackerRequested = true;
        self->windowStackPopTopWindow_();
        return true;
    }
    if (isClaytonClicked(&clayton->menuBotSelectClick, e))
    {
        clayton->shouldShowBotSelect = true;
        self->windowStackPushBotSelectWindow();
        return true;
    }

    if (Clay_PointerOver(CLAY_ID("MenuContainer")))
        return true;

    return false;
}

inline bool WindowStack::processMassEditorWindowEvent(
    WindowStack *self, Clayton *clayton, Clayton_Slider *massSlider, SDL_Event e
)
{
    (void)self;
    if (!clayton || !massSlider)
        return false;

    // Close button (same click semantics as Clayton_Click but without storing new state in structs).
    static bool s_down = false;
    const Clay_ElementId closeId = CLAY_ID("MassEditorCloseBtn");
    const bool over = Clay_PointerOver(closeId);
    const bool isDownEv = (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_FINGERDOWN);
    const bool isUpEv = (e.type == SDL_MOUSEBUTTONUP) || (e.type == SDL_FINGERUP);
    const bool isMoveEv = (e.type == SDL_MOUSEMOTION) || (e.type == SDL_FINGERMOTION);

    if (s_down)
    {
        if (isMoveEv && !over)
            s_down = false;
        if (isUpEv)
        {
            s_down = false;
            // Pop is handled by caller via returning false? We can just clear by requesting close:
            // easiest is to pretend clayton flag, but we don't have one. Instead, pop directly:
            self->windowStackPopTopWindow_();
            return true;
        }
    }
    else
    {
        if (isDownEv && over)
        {
            s_down = true;
            return true;
        }
    }

    // Slider interaction consumes events when active.
    if (ClaytonSlider_ProcessEvent(massSlider, e))
    {
        // Clamp the slider itself (range is already set at init, but keep it safe).
        float v = glm::clamp(massSlider->value, massSlider->minValue, massSlider->maxValue);
        ClaytonSlider_SetValue(massSlider, v);
        return true;
    }

    // Any pointer events inside the window should be consumed to avoid click-through.
    const Clay_ElementId containerId = CLAY_ID("MassEditorContainer");
    const bool overWin = Clay_PointerOver(containerId);
    if (overWin && (isDownEv || isUpEv || isMoveEv))
        return true;

    return false;
}

inline bool WindowStack::processTrackerEditorWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleEditorWindowEvent(tracker, e);
}

inline bool WindowStack::processTrackerInstrumentsWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleInstrumentsWindowEvent(tracker, e);
}

inline bool WindowStack::processTrackerSongSettingsWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleSongSettingsWindowEvent(tracker, e);
}

inline bool WindowStack::processTrackerInstrumentEditorWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleInstrumentEditorWindowEvent(tracker, e);
}

inline bool WindowStack::processTrackerInstrumentColorWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleInstrumentColorWindowEvent(tracker, e);
}

inline bool WindowStack::processTrackerOperatorEditorWindowEvent(WindowStack * /*self*/, Tracker *tracker, SDL_Event e)
{
    return Tracker_HandleOperatorEditorWindowEvent(tracker, e);
}

inline void WindowStack::renderMassEditorWindow(Clayton *clayton, Clayton_Slider *massSlider)
{
    if (!clayton || !massSlider)
        return;

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;

    CLAY(
        CLAY_ID("MassEditorContainer"),
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
        CLAY(CLAY_ID("MassEditorWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            // Title row with close button
            CLAY(
                CLAY_ID("MassEditorTitleRow"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {0, 0, 5, 0},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                }
            )
            {
                CLAY_TEXT(CLAY_STRING("Change Ball Mass"), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY(CLAY_ID("MassEditorTitleDivider"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}}}) {}
                CLAY(CLAY_ID("MassEditorCloseBtn"), CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("x"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }

            CLAY(
                CLAY_ID("MassEditorBody"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                }
            )
            {
                ClaytonSlider_Render(massSlider, clayton, "Mass", "kg");
            }
        }
    }
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

inline void WindowStack::renderOilStatusWindow(Clayton *clayton, CarouselState *carousel, const OilStatusUI *oilStatus)
{
    buildOilStatusWindowClay(clayton, carousel ? carousel->bank : 0.0f, oilStatus);
}

inline void WindowStack::renderHousesWindow(Clayton *clayton, HouseCarouselState *houses, float deltaTime)
{
    buildHousesWindowClay(clayton, houses, deltaTime);
}

inline void WindowStack::renderBotSelectWindow(Clayton *clayton, BotCarouselState *bots, float deltaTime)
{
    buildBotsWindowClay(clayton, bots, deltaTime);
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

inline void WindowStack::renderMenuWindow(Clayton *clayton, bool showGoToSchool, bool showTracker)
{
    buildMenuWindowClay(clayton, showGoToSchool, showTracker);
}

inline void WindowStack::renderTrackerEditorWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildEditor(tracker, clayton);
}

inline void WindowStack::renderTrackerInstrumentsWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildInstrumentsWindow(tracker, clayton);
}

inline void WindowStack::renderTrackerSongSettingsWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildSongSettingsWindow(tracker, clayton);
}

inline void WindowStack::renderTrackerInstrumentEditorWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildInstrumentEditor(tracker, clayton);
}

inline void WindowStack::renderTrackerInstrumentColorWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildInstrumentColorWindow(tracker, clayton);
}

inline void WindowStack::renderTrackerOperatorEditorWindow(Clayton *clayton, Tracker *tracker)
{
    Tracker_BuildOperatorEditor(tracker, clayton);
}

inline void WindowStack::renderBotResultWindow(WindowStack *self, Clayton *clayton)
{
    if (!self || !clayton)
        return;

    const char *title = self->botResultPlayerWon ? "YOU BEAT ANGEL" : "ANGEL WINS";
    char scoreLine[128];
    (void)snprintf(
        scoreLine,
        sizeof(scoreLine),
        "You: %d    Angel: %d",
        self->botResultPlayerScore,
        self->botResultAngelScore
    );
    const char *detail = self->botResultPlayerWon ? "Victory!" : "Defeat.";

    Clay_String titleStr = {
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(title),
        .chars = title,
    };
    Clay_String scoreStr = {
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(scoreLine),
        .chars = scoreLine,
    };
    Clay_String detailStr = {
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(detail),
        .chars = detail,
    };

    CLAY(CLAY_ID("BotResultWindow"), CLAY_THEME_WINDOW_PANEL)
    {
        CLAY_TEXT(titleStr, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_TITLE));
        CLAY_TEXT(scoreStr, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BODY));
        CLAY_TEXT(detailStr, CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_LABEL));

        CLAY(clayton->botResultCloseClick.clayId, CLAY_THEME_BTN_PRIMARY)
        {
            CLAY_TEXT(CLAY_STRING("Continue"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
        }
    }
}
