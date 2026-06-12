#pragma once

#include "./clayton.h"
#include "./claytheme.h"

// new_game_clay.h (module)
// Simple "Play again" window shown at end of a run.
// - No close (X) button; only green action button.
// - Click wiring lives in Clayton so the WindowStack can stay stateless-ish and explicit.

inline void renderNewGameWindow(Clayton *clayton)
{
    Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;
    ClayArena *arena = &clayton->clayArena;
    Clay_String title = ClayArena_FormatString(arena, "%s", clayton->newGameTitle ? clayton->newGameTitle : "TRY AGAIN");
    Clay_String button = ClayArena_FormatString(
        arena,
        "%s",
        clayton->newGameButtonLabel ? clayton->newGameButtonLabel : "TRY AGAIN"
    );

    // Container exists for pointer-hit testing in WindowStack.
    CLAY(
        CLAY_ID("NewGameWindowContainer"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {0, 0, 0, 0},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }
    )
    {
        CLAY(CLAY_ID("NewGameWindow"), CLAY_THEME_WINDOW_PANEL)
        {
            CLAY(CLAY_ID("NewGameCentered"), {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                }
                    })

            {
                CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));

                CLAY(clayton->playAgainClick.clayId, CLAY_THEME_BTN_SUCCESS)
                {
                    CLAY_TEXT(button, CLAY_TEXT_CONFIG(buttonCfg));
                }
            }
        }
    }
}
