#pragma once

#include "./sounds.h"
#include "../clayton/clayton.h"

inline void buildWavExportLoadingIndicator(
    SoundSettings *self,
    int exportProgress,
    float exportedSeconds,
    float exportTotalSeconds,
    int sampleRate
)
{
    if (!self->wavExportInProgress)
    {
        return;
    }

    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;
    Clay_TextElementConfig bodyFontCfg = CLAY_THEME_TEXT_BODY;

    // Full-screen overlay
    CLAY(
        CLAY_ID("WavExportOverlay"),
        {
            .layout =
                {
                    .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                },
            .backgroundColor = {0, 0, 0, 0},
        }
    )
    {
        // Modal window
        CLAY(
            CLAY_ID("WavExportModal"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(0.7f), CLAY_SIZING_FIT(0)},
                        .padding = {30, 30, 30, 30},
                        .childGap = 20,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .backgroundColor = {40, 40, 60, 255},
                .cornerRadius = {15, 15, 15, 15},
            }
        )
        {
            CLAY_TEXT(CLAY_STRING("Caching Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));

            // Status text
            Clay_String statusStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(self->wavExportStatus),
                .chars = self->wavExportStatus,
            };
            if (statusStr.length > 0)
            {
                CLAY_TEXT(statusStr, CLAY_TEXT_CONFIG(bodyFontCfg));
            }
            else
            {
                CLAY_TEXT(CLAY_STRING("Preparing audio..."), CLAY_TEXT_CONFIG(bodyFontCfg));
            }

            // Progress bar background
            CLAY(
                CLAY_ID("WavExportProgressBg"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(30)},
                        },
                    .backgroundColor = {40, 40, 40, 255},
                    .cornerRadius = {5, 5, 5, 5},
                }
            )
            {
                // Progress bar fill - clamp to 0.0-1.0 range
                float progress = exportProgress / 100.0f;
                if (progress < 0.0f)
                    progress = 0.0f;
                if (progress > 1.0f)
                    progress = 1.0f;
                CLAY(
                    CLAY_ID("WavExportProgressFill"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW(0)},
                            },
                        .backgroundColor = {50, 200, 50, 255},
                        .cornerRadius = {5, 5, 5, 5},
                    }
                ){};
            }

            // Progress percentage text with time info
            char progressText[128];
            if (exportTotalSeconds > 0)
            {
                snprintf(
                    progressText,
                    sizeof(progressText),
                    "Progress: %d%% (%.1fs exported / %.1fs total)",
                    exportProgress,
                    exportedSeconds,
                    exportTotalSeconds
                );
            }
            else
            {
                snprintf(progressText, sizeof(progressText), "Progress: %d%%", exportProgress);
            }
            Clay_String progressStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(progressText),
                .chars = progressText,
            };
            CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(bodyFontCfg));

            // Animated loading dots
            uint32_t tick = SDL_GetTicks64() / 500; // Change every 500ms
            char dots[5];
            int dotCount = tick % 4;
            for (int i = 0; i < dotCount; i++)
                dots[i] = '.';
            dots[dotCount] = '\0';

            char loadingText[64];
            snprintf(loadingText, sizeof(loadingText), "Please wait%s", dots);
            Clay_String loadingStr = {
                .isStaticallyAllocated = false,
                .length = (int)strlen(loadingText),
                .chars = loadingText,
            };
            CLAY_TEXT(loadingStr, CLAY_TEXT_CONFIG(bodyFontCfg));
        }
    }
}
