#pragma once

#include <SDL.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

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

// Configurable yield interval for WAV export (in samples)
// At 44100 Hz, 4410 samples = 100ms of audio per step
// Smaller values yield more frequently (smoother UI) but add overhead
#ifndef XFM_EXPORT_YIELD_SAMPLES
#define ADAPTIVE_AUDIO_EXPORT_YIELD_SAMPLES 4410
#else
#define ADAPTIVE_AUDIO_EXPORT_YIELD_SAMPLES XFM_EXPORT_YIELD_SAMPLES
#endif

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
// Each song/SFX goes through BEGIN -> STEP (repeated) -> FINALIZE phases
enum AdaptiveAudioExportStep {
    EXPORT_STEP_IDLE,
    EXPORT_STEP_CREATE_MODULES,

    // Song 1-4: BEGIN -> STEP -> FINALIZE
    EXPORT_STEP_SONG_1_BEGIN,
    EXPORT_STEP_SONG_1_STEP,
    EXPORT_STEP_SONG_1_FINALIZE,

    EXPORT_STEP_SONG_2_BEGIN,
    EXPORT_STEP_SONG_2_STEP,
    EXPORT_STEP_SONG_2_FINALIZE,

    EXPORT_STEP_SONG_3_BEGIN,
    EXPORT_STEP_SONG_3_STEP,
    EXPORT_STEP_SONG_3_FINALIZE,

    EXPORT_STEP_SONG_4_BEGIN,
    EXPORT_STEP_SONG_4_STEP,
    EXPORT_STEP_SONG_4_FINALIZE,

    // SFX 1-6: BEGIN -> STEP -> FINALIZE
    EXPORT_STEP_SFX_1_BEGIN,
    EXPORT_STEP_SFX_1_STEP,
    EXPORT_STEP_SFX_1_FINALIZE,

    EXPORT_STEP_SFX_2_BEGIN,
    EXPORT_STEP_SFX_2_STEP,
    EXPORT_STEP_SFX_2_FINALIZE,

    EXPORT_STEP_SFX_3_BEGIN,
    EXPORT_STEP_SFX_3_STEP,
    EXPORT_STEP_SFX_3_FINALIZE,

    EXPORT_STEP_SFX_4_BEGIN,
    EXPORT_STEP_SFX_4_STEP,
    EXPORT_STEP_SFX_4_FINALIZE,

    EXPORT_STEP_SFX_5_BEGIN,
    EXPORT_STEP_SFX_5_STEP,
    EXPORT_STEP_SFX_5_FINALIZE,

    EXPORT_STEP_SFX_6_BEGIN,
    EXPORT_STEP_SFX_6_STEP,
    EXPORT_STEP_SFX_6_FINALIZE,

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

    // Yieldable export state for current song/SFX
    xfm_export_song_state songExportState;
    xfm_export_sfx_state sfxExportState;

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
    int exportTotalSamples;     // Total samples across all songs/SFX (for unified progress)
    int exportRenderedSamples;  // Cumulative samples rendered across all songs/SFX

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
    self->exportTotalSamples = 0;
    self->exportRenderedSamples = 0;

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

        // Zero out yieldable export states
        memset(&self->songExportState, 0, sizeof(xfm_export_song_state));
        memset(&self->sfxExportState, 0, sizeof(xfm_export_sfx_state));

        // Calculate total expected duration AND total samples from Furnace pattern data
        // Duration = rows × speed / tick_rate
        // Samples = duration × sample_rate
        float totalSeconds = 0.0f;
        int totalSamples = 0;
        const char* songPatternsInit[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
        int songSpeedInit[] = { 6, 8, 6, 6 };
        int songTickRateInit = 60;
        for (int i = 0; i < 4; i++) {
            int rows = 0;
            const char* p = songPatternsInit[i];
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            float songSec = (float)rows * songSpeedInit[i] / songTickRateInit;
            totalSeconds += songSec;
            totalSamples += (int)(songSec * sampleRate);
        }
        const char* sfxPatternsInit[] = {
            SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
            SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
        };
        int sfxSpeedInit = 3;
        for (int i = 0; i < 6; i++) {
            int rows = 0;
            const char* p = sfxPatternsInit[i];
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            float sfxSec = (float)rows * sfxSpeedInit / songTickRateInit;
            totalSeconds += sfxSec;
            totalSamples += (int)(sfxSec * sampleRate);
        }
        self->exportTotalSeconds = totalSeconds;
        self->exportedSeconds = 0.0f;
        self->exportTotalSamples = totalSamples;
        self->exportRenderedSamples = 0;

        printf("[AdaptiveAudio] Total expected duration: %.1fs, total samples: %d\n", totalSeconds, totalSamples);

        // Set initial status so UI shows something immediately
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Starting audio cache...");
    }

    // Declare pattern arrays once (used by macros)
    const char* songPatternsArr[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
    int songTicksArr[] = { 6, 8, 6, 6 };
    const char* sfxPatternsArr[] = {
        SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
        SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
    };
    int sfxIdsArr[] = { 0, 1, 2, 3, 4, 5 };

    // Helper: update unified progress bar
    #define UPDATE_PROGRESS \
        if (self->exportTotalSamples > 0) { \
            self->exportProgress = (int)(self->exportRenderedSamples * 100 / self->exportTotalSamples); \
        }

    // Helper macro for song export BEGIN phase
    // Creates a FRESH module for each song to avoid YM3438 state leakage
    // (phase, envelopes, LFO) - matches original fix from commit 845ff55
    #define SONG_BEGIN(songIdx) \
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching song %d/4...", songIdx + 1); \
        self->exportCurrent = songIdx; \
        UPDATE_PROGRESS; \
        self->songModule = xfm_module_create(self->exportSampleRate, self->bufferSize, XFM_CHIP_YM3438); \
        if (!self->songModule) { \
            printf("[AdaptiveAudio] ERROR: Failed to create song module for song %d\n", songIdx + 1); \
            self->songBuffers[songIdx] = NULL; \
            self->songBufferSizes[songIdx] = 0; \
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 2); \
            break; \
        } \
        xfm_patch_set(self->songModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x09, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0A, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0B, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0C, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0D, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0E, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x10, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0x11, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438); \
        xfm_song_declare(self->songModule, songIdx + 1, songPatternsArr[songIdx], 60, songTicksArr[songIdx]); \
        if (xfm_export_song_begin(&self->songExportState, self->songModule, songIdx + 1, ADAPTIVE_AUDIO_EXPORT_YIELD_SAMPLES) != 0) { \
            printf("[AdaptiveAudio] ERROR: xfm_export_song_begin failed for song %d\n", songIdx + 1); \
            self->songBuffers[songIdx] = NULL; \
            self->songBufferSizes[songIdx] = 0; \
            xfm_module_destroy(self->songModule); \
            self->songModule = NULL; \
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 2); \
            break; \
        } \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Helper macro for song export STEP phase
    // Yields each frame during rendering - updates unified progress
    #define SONG_STEP(songIdx) \
        if (xfm_export_song_step(&self->songExportState) != 0) { \
            printf("[AdaptiveAudio] ERROR: xfm_export_song_step failed for song %d\n", songIdx + 1); \
            self->songBuffers[songIdx] = NULL; \
            self->songBufferSizes[songIdx] = 0; \
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 2); \
            break; \
        } \
        if (!self->songExportState.done) { \
            int currentTotal = self->exportRenderedSamples + self->songExportState.samples_rendered; \
            if (self->exportTotalSamples > 0) { \
                self->exportProgress = (int)(currentTotal * 100 / self->exportTotalSamples); \
            } \
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching song %d/4... %d%%", songIdx + 1, self->exportProgress); \
            return false; /* Still rendering, yield and come back next frame */ \
        } \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Helper macro for song export FINALIZE phase
    // Destroys the module - fresh one created for next song (avoids YM3438 state leakage)
    #define SONG_FINALIZE(songIdx) \
        self->songBuffers[songIdx] = xfm_export_song_finalize(&self->songExportState, &self->songBufferSizes[songIdx]); \
        xfm_export_song_cleanup(&self->songExportState); \
        if (self->songBuffers[songIdx]) { \
            printf("[AdaptiveAudio] Song %d exported: %d bytes\n", songIdx + 1, self->songBufferSizes[songIdx]); \
            const char* p = songPatternsArr[songIdx]; \
            int rows = 0; \
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; \
            while (*p >= '0' && *p <= '9') { rows = rows * 10 + (*p - '0'); p++; } \
            float songSeconds = (float)rows * songTicksArr[songIdx] / 60.0f; \
            self->exportedSeconds += songSeconds; \
            self->exportRenderedSamples += self->songExportState.samples_rendered; \
            UPDATE_PROGRESS; \
        } else { \
            printf("[AdaptiveAudio] ERROR: Failed to export song %d\n", songIdx + 1); \
        } \
        xfm_module_destroy(self->songModule); \
        self->songModule = NULL; \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Helper macro for SFX export BEGIN phase
    // Resets SFX module state before each SFX (matches original behavior)
    #define SFX_BEGIN(sfxIdx, sfxId) \
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching SFX %d/6...", sfxIdx + 1); \
        self->exportCurrent = 4 + sfxIdx; \
        UPDATE_PROGRESS; \
        xfm_module_reset_state(self->sfxModule); \
        xfm_module_set_lfo(self->sfxModule, true, 5); \
        xfm_set_auto_off_delay(self->sfxModule, 0.3f); \
        xfm_patch_set(self->sfxModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x12, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438); \
        xfm_sfx_declare(self->sfxModule, sfxId, sfxPatternsArr[sfxIdx], 60, 3); \
        if (xfm_export_sfx_begin(&self->sfxExportState, self->sfxModule, sfxId, ADAPTIVE_AUDIO_EXPORT_YIELD_SAMPLES) != 0) { \
            printf("[AdaptiveAudio] ERROR: xfm_export_sfx_begin failed for SFX %d\n", sfxIdx + 1); \
            self->sfxBuffers[sfxIdx] = NULL; \
            self->sfxBufferSizes[sfxIdx] = 0; \
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 2); \
            break; \
        } \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Helper macro for SFX export STEP phase
    // Yields each frame during rendering - updates unified progress
    #define SFX_STEP(sfxIdx) \
        if (xfm_export_sfx_step(&self->sfxExportState) != 0) { \
            printf("[AdaptiveAudio] ERROR: xfm_export_sfx_step failed for SFX %d\n", sfxIdx + 1); \
            self->sfxBuffers[sfxIdx] = NULL; \
            self->sfxBufferSizes[sfxIdx] = 0; \
            self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 2); \
            break; \
        } \
        if (!self->sfxExportState.done) { \
            int currentTotal = self->exportRenderedSamples + self->sfxExportState.samples_rendered; \
            if (self->exportTotalSamples > 0) { \
                self->exportProgress = (int)(currentTotal * 100 / self->exportTotalSamples); \
            } \
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching SFX %d/6... %d%%", sfxIdx + 1, self->exportProgress); \
            return false; /* Still rendering, yield and come back next frame */ \
        } \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Helper macro for SFX export FINALIZE phase
    // Adds to cumulative progress
    #define SFX_FINALIZE(sfxIdx) \
        self->sfxBuffers[sfxIdx] = xfm_export_sfx_finalize(&self->sfxExportState, &self->sfxBufferSizes[sfxIdx]); \
        xfm_export_sfx_cleanup(&self->sfxExportState); \
        if (self->sfxBuffers[sfxIdx]) { \
            printf("[AdaptiveAudio] SFX %d exported: %d bytes\n", sfxIdx, self->sfxBufferSizes[sfxIdx]); \
            const char* p = sfxPatternsArr[sfxIdx]; \
            int rows = 0; \
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; \
            while (*p >= '0' && *p <= '9') { rows = rows * 10 + (*p - '0'); p++; } \
            float sfxSeconds = (float)rows * 3 / 60.0f; \
            self->exportedSeconds += sfxSeconds; \
            self->exportRenderedSamples += self->sfxExportState.samples_rendered; \
            UPDATE_PROGRESS; \
        } else { \
            printf("[AdaptiveAudio] ERROR: Failed to export SFX %d\n", sfxIdx + 1); \
        } \
        self->exportStep = (AdaptiveAudioExportStep)(self->exportStep + 1); \
        break;

    // Process one step per call
    switch (self->exportStep) {
        case EXPORT_STEP_CREATE_MODULES: {
            // Create SFX module only (persistent, reset before each SFX)
            // Songs get their own fresh modules (created in SONG_BEGIN, destroyed in SONG_FINALIZE)
            self->sfxModule = xfm_module_create(self->exportSampleRate, self->bufferSize, XFM_CHIP_YM3438);
            if (!self->sfxModule) {
                printf("[AdaptiveAudio] ERROR: Failed to create SFX module\n");
                self->state = ADAPTIVE_DECIDING;
                self->exportStep = EXPORT_STEP_DONE;
                return true;
            }
            self->exportStep = EXPORT_STEP_SONG_1_BEGIN;
            break;
        }

        // Song 1-4: each gets a FRESH module (created in BEGIN, destroyed in FINALIZE)
        // This avoids YM3438 state leakage between songs (commit 845ff55)
        case EXPORT_STEP_SONG_1_BEGIN: SONG_BEGIN(0)
        case EXPORT_STEP_SONG_1_STEP: SONG_STEP(0)
        case EXPORT_STEP_SONG_1_FINALIZE: SONG_FINALIZE(0)

        case EXPORT_STEP_SONG_2_BEGIN: SONG_BEGIN(1)
        case EXPORT_STEP_SONG_2_STEP: SONG_STEP(1)
        case EXPORT_STEP_SONG_2_FINALIZE: SONG_FINALIZE(1)

        case EXPORT_STEP_SONG_3_BEGIN: SONG_BEGIN(2)
        case EXPORT_STEP_SONG_3_STEP: SONG_STEP(2)
        case EXPORT_STEP_SONG_3_FINALIZE: SONG_FINALIZE(2)

        case EXPORT_STEP_SONG_4_BEGIN: SONG_BEGIN(3)
        case EXPORT_STEP_SONG_4_STEP: SONG_STEP(3)
        case EXPORT_STEP_SONG_4_FINALIZE: SONG_FINALIZE(3)

        // SFX 1-6: uses persistent SFX module (reset before each)
        case EXPORT_STEP_SFX_1_BEGIN: SFX_BEGIN(0, 0)
        case EXPORT_STEP_SFX_1_STEP: SFX_STEP(0)
        case EXPORT_STEP_SFX_1_FINALIZE: SFX_FINALIZE(0)

        case EXPORT_STEP_SFX_2_BEGIN: SFX_BEGIN(1, 1)
        case EXPORT_STEP_SFX_2_STEP: SFX_STEP(1)
        case EXPORT_STEP_SFX_2_FINALIZE: SFX_FINALIZE(1)

        case EXPORT_STEP_SFX_3_BEGIN: SFX_BEGIN(2, 2)
        case EXPORT_STEP_SFX_3_STEP: SFX_STEP(2)
        case EXPORT_STEP_SFX_3_FINALIZE: SFX_FINALIZE(2)

        case EXPORT_STEP_SFX_4_BEGIN: SFX_BEGIN(3, 3)
        case EXPORT_STEP_SFX_4_STEP: SFX_STEP(3)
        case EXPORT_STEP_SFX_4_FINALIZE: SFX_FINALIZE(3)

        case EXPORT_STEP_SFX_5_BEGIN: SFX_BEGIN(4, 4)
        case EXPORT_STEP_SFX_5_STEP: SFX_STEP(4)
        case EXPORT_STEP_SFX_5_FINALIZE: SFX_FINALIZE(4)

        case EXPORT_STEP_SFX_6_BEGIN: SFX_BEGIN(5, 5)
        case EXPORT_STEP_SFX_6_STEP: SFX_STEP(5)
        case EXPORT_STEP_SFX_6_FINALIZE: SFX_FINALIZE(5)

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
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching complete!");
            self->state = ADAPTIVE_WAV;
            self->useWavMode = true;
            printf("[AdaptiveAudio] WAV export complete!\n");
            return true;  // All done
        }

        default:
            return false;  // Not started yet
    }

    #undef UPDATE_PROGRESS
    #undef SONG_BEGIN
    #undef SONG_STEP
    #undef SONG_FINALIZE
    #undef SFX_BEGIN
    #undef SFX_STEP
    #undef SFX_FINALIZE

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
            .backgroundColor = {0, 0, 0, 0}, // Transparent
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
                CLAY_TEXT(CLAY_STRING("Low Performance Detected"), CLAY_TEXT_CONFIG(titleFontCfg));
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
                        CLAY_TEXT(CLAY_STRING("Use Synth"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                    
                    // Use Cached button
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
                        CLAY_TEXT(CLAY_STRING("Use Cached"), CLAY_TEXT_CONFIG(buttonFontCfg));
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
                        CLAY_TEXT(CLAY_STRING("Disable"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }
                
                // Explanation text
                CLAY_TEXT(CLAY_STRING("Synth: Real-time OPN chip synthesis (no preload, more CPU)"),
                          CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("Cached: Pre-generated audio blobs (needs caching, lighter on CPU)"),
                          CLAY_TEXT_CONFIG(bodyFontCfg));
            } else if (self->state == ADAPTIVE_EXPORTING) {
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
