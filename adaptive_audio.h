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
    ADAPTIVE_EXPORTING,     // Exporting WAVs to memory
    ADAPTIVE_RESTARTING,    // Restarting sound system with chosen mode
    ADAPTIVE_SYNTH,         // Using synth mode (good FPS)
    ADAPTIVE_WAV,           // Using WAV mode (exported to memory)
    ADAPTIVE_DISABLED       // Audio disabled
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
    
    // WAV export
    void* songBuffers[4];  // 4 songs (malloc'd WAV data)
    int songBufferSizes[4];
    void* sfxBuffers[6];  // 6 SFX (malloc'd WAV data)
    int sfxBufferSizes[6];
    int exportProgress;  // 0-100
    int exportTotal;
    int exportCurrent;
    char exportStatus[128];
    
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
}

void AdaptiveAudio_ExportWAV(AdaptiveAudioSystem* self, int sampleRate)
{
    printf("[AdaptiveAudio] Starting WAV export at %d Hz...\n", sampleRate);
    self->state = ADAPTIVE_EXPORTING;
    self->exportProgress = 0;
    self->exportCurrent = 0;
    self->exportTotal = 10;  // 4 songs + 6 SFX
    
    int bufferSize = 4096;
    
    // Create module for export
    xfm_module* exportModule = xfm_module_create(sampleRate, bufferSize, XFM_CHIP_YM3438);
    if (!exportModule) {
        printf("[AdaptiveAudio] ERROR: Failed to create export module\n");
        self->state = ADAPTIVE_DECIDING;
        return;
    }
    
    // Load all patches
    xfm_patch_set(exportModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x09, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0A, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0B, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0C, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0D, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0E, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x10, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x11, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438);
    
    // Export songs
    const char* songPatterns[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
    int songTicks[] = { 6, 8, 6, 6 };
    
    for (int i = 0; i < 4; i++) {
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Exporting song %d/4...", i + 1);
        self->exportCurrent = i;
        self->exportProgress = (i * 100) / self->exportTotal;
        
        xfm_song_declare(exportModule, i + 1, songPatterns[i], 60, songTicks[i]);
        self->songBuffers[i] = xfm_export_song_to_memory(exportModule, i + 1, &self->songBufferSizes[i]);
        
        if (self->songBuffers[i]) {
            printf("[AdaptiveAudio] Song %d exported: %d bytes\n", i + 1, self->songBufferSizes[i]);
        } else {
            printf("[AdaptiveAudio] ERROR: Failed to export song %d\n", i + 1);
        }
    }
    
    // Load SFX patches
    xfm_patch_set(exportModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(exportModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
    
    // Export SFX
    const char* sfxPatterns[] = {
        SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
        SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
    };
    int sfxIds[] = { 0, 1, 2, 3, 4, 5 };
    
    for (int i = 0; i < 6; i++) {
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Exporting SFX %d/6...", i + 1);
        self->exportCurrent = 4 + i;
        self->exportProgress = ((4 + i) * 100) / self->exportTotal;
        
        xfm_sfx_declare(exportModule, sfxIds[i], sfxPatterns[i], 60, 3);
        self->sfxBuffers[i] = xfm_export_sfx_to_memory(exportModule, sfxIds[i], &self->sfxBufferSizes[i]);
        
        if (self->sfxBuffers[i]) {
            printf("[AdaptiveAudio] SFX %d exported: %d bytes\n", i + 1, self->sfxBufferSizes[i]);
        } else {
            printf("[AdaptiveAudio] ERROR: Failed to export SFX %d\n", i + 1);
        }
    }
    
    xfm_module_destroy(exportModule);
    
    self->exportProgress = 100;
    snprintf(self->exportStatus, sizeof(self->exportStatus), "Export complete!");
    self->state = ADAPTIVE_WAV;
    self->useWavMode = true;
    printf("[AdaptiveAudio] WAV export complete!\n");
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
                char progressText[64];
                int len = snprintf(progressText, sizeof(progressText), "Progress: %d%% (%d/%d)", 
                                   self->exportProgress, self->exportCurrent, self->exportTotal);
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
