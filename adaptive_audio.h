#pragma once

#include <SDL.h>
#include <cstdio>
#include <cstdint>

#include "./../eggsfm/xfm_api.h"
#include "./../eggsfm/xfm_wavplay.h"
#include "./../eggsfm/xfm_export.h"
#include "./sounds/songs_data.h"

#include <clay.h>
#include "./clayton/clayton_click.h"

// -----------------------------------------------------------------------------
// Adaptive Audio Quality System
// Monitors FPS and offers WAV caching if performance is low
// -----------------------------------------------------------------------------

enum AdaptiveAudioState {
    ADAPTIVE_MONITORING,    // Monitoring FPS at startup
    ADAPTIVE_DECIDING,      // FPS is low, showing modal
    ADAPTIVE_EXPORTING,     // Exporting WAVs to memory (yieldable)
    ADAPTIVE_RESTARTING,    // Restarting sound system with chosen mode
    ADAPTIVE_SYNTH,         // Using synth mode (good FPS)
    ADAPTIVE_WAV,           // Using WAV mode (exported to memory)
    ADAPTIVE_DISABLED       // Audio disabled
};

// Export step tracking for yieldable export
enum AdaptiveAudioExportStep {
    EXPORT_STEP_IDLE,
    EXPORT_STEP_CREATE_MODULES,
    EXPORT_STEP_SONG_1,
    EXPORT_STEP_SONG_2,
    EXPORT_STEP_SONG_3,
    EXPORT_STEP_SONG_4,
    EXPORT_STEP_SFX_1,
    EXPORT_STEP_SFX_2,
    EXPORT_STEP_SFX_3,
    EXPORT_STEP_SFX_4,
    EXPORT_STEP_SFX_5,
    EXPORT_STEP_SFX_6,
    EXPORT_STEP_CLEANUP,
    EXPORT_STEP_DONE
};

struct AdaptiveAudioSystem {
    AdaptiveAudioState state;

    // FPS monitoring
    float monitoringStartTime;
    float monitoringDuration;  // How long to monitor (seconds)
    float accumulatedFps;
    int fpsSampleCount;
    float currentFps;
    float fpsThreshold;  // Below this triggers modal

    // WAV export (yieldable state machine)
    AdaptiveAudioExportStep exportStep;  // Current step in export state machine
    int exportSampleRate;
    int bufferSize;
    xfm_module* sfxModule;  // Persistent SFX module across yield calls
    int currentSongIndex;   // Which song we're on (0-3)
    int currentSfxIndex;    // Which SFX we're on (0-5)
    xfm_module* songModule; // Temporary song module (destroyed after each song)

    void* songBuffers[4];  // 4 songs (malloc'd WAV data)
    int songBufferSizes[4];
    void* sfxBuffers[6];  // 6 SFX (malloc'd WAV data)
    int sfxBufferSizes[6];
    int exportProgress;  // 0-100
    int exportTotal;
    int exportCurrent;
    char exportStatus[128];
    float exportTotalSeconds;   // Total expected duration in seconds
    float exportedSeconds;      // Duration exported so far in seconds

    // UI
    bool showModal;
    Clayton_Click useSynthClick;
    Clayton_Click useWavClick;
    Clayton_Click disableAudioClick;

    // Result
    bool useWavMode;
    bool audioDisabled;
    bool restartRequested;
    bool restartUseWav;
};

// Forward declarations
void AdaptiveAudio_Init(AdaptiveAudioSystem* self, float fpsThreshold);
void AdaptiveAudio_Update(AdaptiveAudioSystem* self, float deltaTime, float currentFps);
void AdaptiveAudio_RenderUI(AdaptiveAudioSystem* self);
bool AdaptiveAudio_ProcessEvent(AdaptiveAudioSystem* self, SDL_Event event);
void AdaptiveAudio_GenerateWAV(AdaptiveAudioSystem* self, int sampleRate);
void AdaptiveAudio_Cleanup(AdaptiveAudioSystem* self);

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

void AdaptiveAudio_Init(AdaptiveAudioSystem* self, float fpsThreshold)
{
    self->state = ADAPTIVE_MONITORING;
    self->monitoringStartTime = 0.0f;
    self->monitoringDuration = 5.0f;  // Monitor for 5 seconds
    self->accumulatedFps = 0.0f;
    self->fpsSampleCount = 0;
    self->currentFps = 60.0f;
    self->fpsThreshold = fpsThreshold;

    for (int i = 0; i < 4; i++) {
        self->songBuffers[i] = NULL;
        self->songBufferSizes[i] = 0;
    }
    for (int i = 0; i < 6; i++) {
        self->sfxBuffers[i] = NULL;
        self->sfxBufferSizes[i] = 0;
    }
    self->exportProgress = 0;
    self->exportTotal = 10;  // 4 songs + 6 SFX
    self->exportCurrent = 0;
    self->exportStatus[0] = '\0';
    self->exportTotalSeconds = 0.0f;
    self->exportedSeconds = 0.0f;

    // Initialize export state machine
    self->exportStep = EXPORT_STEP_IDLE;
    self->exportSampleRate = 0;
    self->bufferSize = 256;
    self->sfxModule = NULL;
    self->currentSongIndex = 0;
    self->currentSfxIndex = 0;
    self->songModule = NULL;

    self->showModal = false;
    self->useWavMode = false;
    self->audioDisabled = false;
    self->restartRequested = false;
    self->restartUseWav = false;

    initClaytonClick(&self->useSynthClick, "adaptiveUseSynth");
    initClaytonClick(&self->useWavClick, "adaptiveUseWav");
    initClaytonClick(&self->disableAudioClick, "adaptiveDisableAudio");
    
    printf("[AdaptiveAudio] Initialized, monitoring FPS (threshold: %.1f)\n", fpsThreshold);
}

void AdaptiveAudio_Update(AdaptiveAudioSystem* self, float deltaTime, float currentFps)
{
    if (self->state == ADAPTIVE_DISABLED || self->state == ADAPTIVE_RESTARTING) return;
    
    if (self->state == ADAPTIVE_MONITORING) {
        if (self->monitoringStartTime == 0.0f) {
            self->monitoringStartTime = (float)SDL_GetTicks64() / 1000.0f;
        }
        
        self->accumulatedFps += currentFps;
        self->fpsSampleCount++;
        self->currentFps = currentFps;
        
        float elapsed = (float)SDL_GetTicks64() / 1000.0f - self->monitoringStartTime;
        
        if (elapsed >= self->monitoringDuration) {
            float avgFps = self->accumulatedFps / self->fpsSampleCount;
            printf("[AdaptiveAudio] Monitoring complete. Avg FPS: %.2f, Threshold: %.2f\n", 
                   avgFps, self->fpsThreshold);
            
            if (avgFps < self->fpsThreshold) {
                // Performance is low, show modal
                self->state = ADAPTIVE_DECIDING;
                self->showModal = true;
                printf("[AdaptiveAudio] Low FPS detected! Showing options modal.\n");
            } else {
                // Performance is good, use synth mode
                self->state = ADAPTIVE_SYNTH;
                self->useWavMode = false;
                printf("[AdaptiveAudio] FPS is good, using synth mode.\n");
            }
        }
    }
}

void AdaptiveAudio_Cleanup(AdaptiveAudioSystem* self)
{
    for (int i = 0; i < 4; i++) {
        if (self->songBuffers[i]) {
            free(self->songBuffers[i]);
            self->songBuffers[i] = NULL;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (self->sfxBuffers[i]) {
            free(self->sfxBuffers[i]);
            self->sfxBuffers[i] = NULL;
        }
    }
    // Clean up any lingering export modules
    if (self->sfxModule) {
        xfm_module_destroy(self->sfxModule);
        self->sfxModule = NULL;
    }
    if (self->songModule) {
        xfm_module_destroy(self->songModule);
        self->songModule = NULL;
    }
    // Reset export state
    self->exportTotalSeconds = 0.0f;
    self->exportedSeconds = 0.0f;
}

// Yieldable WAV export - call this every frame until it returns true
// Returns false while still exporting, true when all songs and SFX are exported
bool AdaptiveAudio_ExportWAV(AdaptiveAudioSystem* self, int sampleRate)
{
    // Initialize export on first call
    if (self->exportStep == EXPORT_STEP_IDLE) {
        printf("[AdaptiveAudio] Starting WAV export at %d Hz...\n", sampleRate);
        self->state = ADAPTIVE_EXPORTING;
        self->exportProgress = 0;
        self->exportCurrent = 0;
        self->exportTotal = 10;  // 4 songs + 6 SFX
        self->exportSampleRate = sampleRate;
        self->currentSongIndex = 0;
        self->currentSfxIndex = 0;
        self->sfxModule = NULL;
        self->songModule = NULL;
        self->exportStep = EXPORT_STEP_CREATE_MODULES;
        
        // Calculate total expected duration in seconds from Furnace pattern data
        // Duration = rows × speed / tick_rate
        float totalSeconds = 0.0f;
        const char* songPatterns[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
        int songSpeed[] = { 6, 8, 6, 6 };  // steps per row
        int songTickRate = 60;  // steps per second (same for all songs)
        for (int i = 0; i < 4; i++) {
            // Parse row count from pattern (first line) - skip leading whitespace/newlines
            int rows = 0;
            const char* p = songPatterns[i];
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            totalSeconds += (float)rows * songSpeed[i] / songTickRate;
        }
        const char* sfxPatterns[] = {
            SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
            SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
        };
        int sfxSpeed = 3;  // steps per row for SFX
        for (int i = 0; i < 6; i++) {
            int rows = 0;
            const char* p = sfxPatterns[i];
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            totalSeconds += (float)rows * sfxSpeed / songTickRate;
        }
        self->exportTotalSeconds = totalSeconds;
        self->exportedSeconds = 0.0f;

        printf("[AdaptiveAudio] Total expected duration: %.1fs\n", totalSeconds);
        
        // Set initial status so UI shows something immediately
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Starting WAV export...");
    }

    // Process one step per call
    switch (self->exportStep) {
        case EXPORT_STEP_CREATE_MODULES: {
            // Create SFX module (songs get their own fresh modules)
            self->sfxModule = xfm_module_create(self->exportSampleRate, self->bufferSize, XFM_CHIP_YM3438);
            if (!self->sfxModule) {
                printf("[AdaptiveAudio] ERROR: Failed to create SFX module\n");
                self->state = ADAPTIVE_DECIDING;
                self->exportStep = EXPORT_STEP_DONE;
                return true;  // Failed, but done
            }
            self->exportStep = EXPORT_STEP_SONG_1;
            break;
        }

        case EXPORT_STEP_SONG_1:
        case EXPORT_STEP_SONG_2:
        case EXPORT_STEP_SONG_3:
        case EXPORT_STEP_SONG_4: {
            int songIdx = self->exportStep - EXPORT_STEP_SONG_1;
            const char* songPatterns[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
            int songTicks[] = { 6, 8, 6, 6 };

            snprintf(self->exportStatus, sizeof(self->exportStatus), "Exporting song %d/4...", songIdx + 1);
            self->exportCurrent = songIdx;
            self->exportProgress = (songIdx * 100) / self->exportTotal;

            // Create FRESH module for this song
            self->songModule = xfm_module_create(self->exportSampleRate, self->bufferSize, XFM_CHIP_YM3438);
            if (!self->songModule) {
                printf("[AdaptiveAudio] ERROR: Failed to create song module for song %d\n", songIdx + 1);
                self->songBuffers[songIdx] = NULL;
                self->songBufferSizes[songIdx] = 0;
            } else {
                // Load ALL song patches
                xfm_patch_set(self->songModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x09, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0A, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0B, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0C, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0D, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0E, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x10, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438);
                xfm_patch_set(self->songModule, 0x11, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438);

                xfm_song_declare(self->songModule, songIdx + 1, songPatterns[songIdx], 60, songTicks[songIdx]);
                self->songBuffers[songIdx] = xfm_export_song_to_memory(self->songModule, songIdx + 1, &self->songBufferSizes[songIdx]);

                if (self->songBuffers[songIdx]) {
                    printf("[AdaptiveAudio] Song %d exported: %d bytes\n", songIdx + 1, self->songBufferSizes[songIdx]);
                    // Update progress based on duration in seconds
                    int songSpeed[] = { 6, 8, 6, 6 };
                    const char* songPatterns[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
                    const char* p = songPatterns[songIdx];
                    int rows = 0;
                    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
                    while (*p >= '0' && *p <= '9') {
                        rows = rows * 10 + (*p - '0');
                        p++;
                    }
                    float songSeconds = (float)rows * songSpeed[songIdx] / 60.0f;
                    self->exportedSeconds += songSeconds;
                    printf("[AdaptiveAudio] Song %d: %d rows, %.1fs, total exported: %.1fs, total expected: %.1fs\n", 
                           songIdx + 1, rows, songSeconds, self->exportedSeconds, self->exportTotalSeconds);
                    if (self->exportTotalSeconds > 0) {
                        self->exportProgress = (int)(self->exportedSeconds * 100.0f / self->exportTotalSeconds);
                    }
                } else {
                    printf("[AdaptiveAudio] ERROR: Failed to export song %d\n", songIdx + 1);
                }

                // Destroy module - fresh one created next iteration
                xfm_module_destroy(self->songModule);
                self->songModule = NULL;
            }

            // Advance to next step
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1);
            break;
        }

        case EXPORT_STEP_SFX_1:
        case EXPORT_STEP_SFX_2:
        case EXPORT_STEP_SFX_3:
        case EXPORT_STEP_SFX_4:
        case EXPORT_STEP_SFX_5:
        case EXPORT_STEP_SFX_6: {
            int sfxIdx = self->exportStep - EXPORT_STEP_SFX_1;
            const char* sfxPatterns[] = {
                SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
                SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
            };
            int sfxIds[] = { 0, 1, 2, 3, 4, 5 };

            snprintf(self->exportStatus, sizeof(self->exportStatus), "Exporting SFX %d/6...", sfxIdx + 1);
            self->exportCurrent = 4 + sfxIdx;
            self->exportProgress = ((4 + sfxIdx) * 100) / self->exportTotal;

            // Reset module state before each SFX export
            xfm_module_reset_state(self->sfxModule);
            xfm_module_set_lfo(self->sfxModule, true, 5);
            xfm_set_auto_off_delay(self->sfxModule, 0.3f);

            // Load SFX patches
            xfm_patch_set(self->sfxModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
            xfm_patch_set(self->sfxModule, 0x12, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438);

            xfm_sfx_declare(self->sfxModule, sfxIds[sfxIdx], sfxPatterns[sfxIdx], 60, 3);
            self->sfxBuffers[sfxIdx] = xfm_export_sfx_to_memory(self->sfxModule, sfxIds[sfxIdx], &self->sfxBufferSizes[sfxIdx]);

            if (self->sfxBuffers[sfxIdx]) {
                printf("[AdaptiveAudio] SFX %d exported: %d bytes\n", sfxIdx, self->sfxBufferSizes[sfxIdx]);
                // Update progress based on duration in seconds
                const char* sfxPatterns[] = {
                    SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
                    SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
                };
                const char* p = sfxPatterns[sfxIdx];
                int rows = 0;
                while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
                while (*p >= '0' && *p <= '9') {
                    rows = rows * 10 + (*p - '0');
                    p++;
                }
                float sfxSeconds = (float)rows * 3 / 60.0f;  // speed=3, tick_rate=60
                self->exportedSeconds += sfxSeconds;
                if (self->exportTotalSeconds > 0) {
                    self->exportProgress = (int)(self->exportedSeconds * 100.0f / self->exportTotalSeconds);
                }
            } else {
                printf("[AdaptiveAudio] ERROR: Failed to export SFX %d\n", sfxIdx);
            }

            // Advance to next step
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1);
            break;
        }

        case EXPORT_STEP_CLEANUP: {
            if (self->sfxModule) {
                xfm_module_destroy(self->sfxModule);
                self->sfxModule = NULL;
            }
            self->exportStep = EXPORT_STEP_DONE;
            break;
        }

        case EXPORT_STEP_DONE: {
            self->exportProgress = 100;
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Export complete!");
            self->state = ADAPTIVE_WAV;
            self->useWavMode = true;
            printf("[AdaptiveAudio] WAV export complete!\n");
            return true;  // All done
        }

        default:
            return false;  // Not started yet
    }

    return false;  // Still exporting
}

void AdaptiveAudio_RenderUI(AdaptiveAudioSystem* self)
{
    if (self->state != ADAPTIVE_DECIDING && self->state != ADAPTIVE_EXPORTING) {
        return;
    }
    
    Clay_TextElementConfig titleFontCfg = {
        .textColor = {255, 255, 255, 255},
        .fontId = 2,
        .fontSize = (uint16_t)32,
    };
    
    Clay_TextElementConfig bodyFontCfg = {
        .textColor = {200, 200, 200, 255},
        .fontId = 0,
        .fontSize = (uint16_t)20,
    };
    
    Clay_TextElementConfig buttonFontCfg = {
        .textColor = {255, 255, 255, 255},
        .fontId = 2,
        .fontSize = (uint16_t)24,
    };
    
    // Full-screen overlay
    CLAY(
        CLAY_ID("AdaptiveOverlay"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            },
            .backgroundColor = {0, 0, 0, 200},
        }
    ) {
        // Modal window
        CLAY(
            CLAY_ID("AdaptiveModal"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.7f), CLAY_SIZING_FIT()},
                    .padding = {30, 30, 30, 30},
                    .childGap = 20,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {40, 40, 60, 255},
                .cornerRadius = {15, 15, 15, 15},
            }
        ) {
            if (self->state == ADAPTIVE_DECIDING) {
                // Show options
                CLAY_TEXT(CLAY_STRING("⚠️ Low Performance Detected"), CLAY_TEXT_CONFIG(titleFontCfg));
                CLAY_TEXT(CLAY_STRING("The game is running at a low frame rate. Please choose an option:"), 
                          CLAY_TEXT_CONFIG(bodyFontCfg));
                
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
                ) {
                    // Use Synth button
                    CLAY(
                        self->useSynthClick.clayId,
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                            .backgroundColor = {50, 100, 200, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("🎹 Use Synth"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                    
                    // Use WAV button
                    CLAY(
                        self->useWavClick.clayId,
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                            .backgroundColor = {50, 150, 50, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("🎵 Generate WAV"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                    
                    // Disable Audio button
                    CLAY(
                        self->disableAudioClick.clayId,
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            },
                            .backgroundColor = {150, 50, 50, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    ) {
                        CLAY_TEXT(CLAY_STRING("🔇 Disable"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }
                
                // Explanation text
                CLAY_TEXT(CLAY_STRING("Synth: Real-time synthesis (lower memory, may use more CPU)"), 
                          CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("WAV: Pre-rendered audio (higher memory, better performance)"), 
                          CLAY_TEXT_CONFIG(bodyFontCfg));
            } else if (self->state == ADAPTIVE_EXPORTING) {
                // Show progress
                CLAY_TEXT(CLAY_STRING("🎵 Exporting WAV Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));
                
                // Status text
                Clay_String statusStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)strlen(self->exportStatus),
                    .chars = self->exportStatus,
                };
                CLAY_TEXT(statusStr, CLAY_TEXT_CONFIG(bodyFontCfg));
                
                // Progress bar background
                CLAY(
                    CLAY_ID("AdaptiveProgressBg"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(30)},
                        },
                        .backgroundColor = {40, 40, 40, 255},
                        .cornerRadius = {5, 5, 5, 5},
                    }
                ) {
                    // Progress bar fill
                    float progress = self->exportProgress / 100.0f;
                    CLAY(
                        CLAY_ID("AdaptiveProgressFill"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                            },
                            .backgroundColor = {50, 200, 50, 255},
                            .cornerRadius = {5, 5, 5, 5},
                        }
                    ) {};
                }
                
                // Progress percentage text
                char progressText[128];
                int len = snprintf(progressText, sizeof(progressText), "Progress: %d%% (%.1fs / %.1fs)",
                                   self->exportProgress, self->exportedSeconds, self->exportTotalSeconds);
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

bool AdaptiveAudio_ProcessEvent(AdaptiveAudioSystem* self, SDL_Event event)
{
    if (self->state != ADAPTIVE_DECIDING) {
        return false;
    }
    
    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;
    
    if (!mouseDown && !mouseUp) {
        return false;
    }
    
    if (isClaytonClicked(&self->useSynthClick, event)) {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = false;
        self->restartRequested = true;
        self->restartUseWav = false;
        self->showModal = false;
        printf("[AdaptiveAudio] User chose Synth mode - will restart sound system\n");
        return true;
    }
    
    if (isClaytonClicked(&self->useWavClick, event)) {
        self->state = ADAPTIVE_RESTARTING;
        self->useWavMode = true;
        self->restartRequested = true;
        self->restartUseWav = true;
        self->showModal = false;

        printf("[AdaptiveAudio] User chose WAV mode - will restart sound system\n");
        return true;
    }
    
    if (isClaytonClicked(&self->disableAudioClick, event)) {
        self->state = ADAPTIVE_DISABLED;
        self->audioDisabled = true;
        self->showModal = false;
        printf("[AdaptiveAudio] User disabled audio\n");
        return true;
    }
    
    // Consume events over the modal
    if (Clay_PointerOver(CLAY_ID("AdaptiveOverlay"))) {
        return true;
    }
    
    return false;
}
