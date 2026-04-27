#pragma once

#include "adaptive_audio.h"
#include "../clayton/clayton.h"

void AdaptiveAudio_RenderUI(Clayton *clayton, AdaptiveAudioSystem *self)
{
    if (self->state != ADAPTIVE_DECIDING && self->state != ADAPTIVE_EXPORTING)
    {
        return;
    }

    // Use theme text configs
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyFontCfg = CLAY_THEME_TEXT_BODY;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;

    // Full-screen overlay
    CLAY(CLAY_ID("AdaptiveOverlay"), CLAY_THEME_OVERLAY)
    {
        // Modal window
        CLAY(
            CLAY_ID("AdaptiveModal"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(0.7f), CLAY_SIZING_FIT()},
                        .padding = {30, 30, 30, 30},
                        .childGap = 20,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
            }
        )
        {
            if (self->state == ADAPTIVE_DECIDING)
            {
                // Show options
                Clay_String fpsStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)strlen(self->fpsMessage),
                    .chars = self->fpsMessage,
                };
                CLAY_TEXT(CLAY_STRING("Low Performance Detected"), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY_TEXT(fpsStr, CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("Please choose an option:"), CLAY_TEXT_CONFIG(bodyFontCfg));

                // Buttons row
                CLAY(
                    CLAY_ID("AdaptiveButtons"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 15,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                )
                {
                    // Use Synth button
                    CLAY(clayton->useSynthClick.clayId, CLAY_THEME_BTN_PRIMARY)
                    {
                        CLAY_TEXT(
                            CLAY_STRING("Use Synth"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON)
                        );
                    }

                    // Use Cached button
                    CLAY(clayton->useWavClick.clayId, CLAY_THEME_BTN_SUCCESS)
                    {
                        CLAY_TEXT(
                            CLAY_STRING("Use Cached"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON)
                        );
                    }

                    // Disable Audio button
                    CLAY(
                        clayton->disableAudioClick.clayId,
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = CLAY_COLOR_BTN_DANGER,
                            .cornerRadius = {
                                CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG, CLAY_RADIUS_LG
                            },
                        }
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("Disable"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                    }
                }

                // Explanation text
                CLAY_TEXT(
                    CLAY_STRING("Synth: Real-time OPN chip synthesis (no preload, more CPU)"),
                    CLAY_TEXT_CONFIG(bodyFontCfg)
                );
                CLAY_TEXT(
                    CLAY_STRING(
                        "Cached: Pre-generated audio blobs (needs caching, lighter on CPU)"
                    ),
                    CLAY_TEXT_CONFIG(bodyFontCfg)
                );
            }
            else if (self->state == ADAPTIVE_EXPORTING)
            {
                // Show progress
                CLAY_TEXT(CLAY_STRING("Caching Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));

                // Status text
                Clay_String statusStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)strlen(self->exportStatus),
                    .chars = self->exportStatus,
                };
                CLAY_TEXT(statusStr, CLAY_TEXT_CONFIG(bodyFontCfg));

                // Progress bar background
                CLAY(CLAY_ID("AdaptiveProgressBg"), CLAY_THEME_PROGRESS_BAR_BG)
                {
                    // Progress bar fill
                    float progress = self->exportProgress / 100.0f;
                    CLAY(
                        CLAY_ID("AdaptiveProgressFill"),
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                                },
                            .backgroundColor = CLAY_COLOR_PROGRESS_FILL,
                            .cornerRadius = {
                                CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM, CLAY_RADIUS_SM
                            },
                        }
                    ){};
                }

                // Progress percentage text
                char progressText[128];
                int len = snprintf(
                    progressText,
                    sizeof(progressText),
                    "Progress: %d%% (%.1fs / %.1fs)",
                    self->exportProgress,
                    self->exportedSeconds,
                    self->exportTotalSeconds
                );
                Clay_String progressStr = {
                    .isStaticallyAllocated = false,
                    .length = len,
                    .chars = progressText,
                };
                CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(bodyFontCfg));
            }
        }
    }
}

bool AdaptiveAudio_ProcessEvent2(Clayton *clayton, AdaptiveAudioSystem *self, SDL_Event event)
{
    if (self->state != ADAPTIVE_DECIDING)
    {
        return false;
    }

    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;

    if (!mouseDown && !mouseUp)
    {
        return false;
    }

    if (isClaytonClicked(&clayton->useSynthClick, event))
    {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = false;
        self->restartRequested = true;
        self->restartUseWav = false;
        self->showModal = false;
        printf("[AdaptiveAudio] User chose Synth mode - will restart sound system\n");
        return true;
    }

    if (isClaytonClicked(&clayton->useWavClick, event))
    {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = true;
        self->restartRequested = true;
        self->restartUseWav = true;
        self->showModal = false;

        printf("[AdaptiveAudio] User chose WAV mode - will restart sound system\n");
        return true;
    }

    if (isClaytonClicked(&clayton->disableAudioClick, event))
    {
        self->state = ADAPTIVE_DISABLED;
        self->audioDisabled = true;
        self->showModal = false;
        printf("[AdaptiveAudio] User disabled audio\n");
        return true;
    }

    // Consume events over the modal
    if (Clay_PointerOver(CLAY_ID("AdaptiveOverlay")))
    {
        return true;
    }

    return false;
}