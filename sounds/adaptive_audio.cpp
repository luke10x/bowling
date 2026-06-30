#include <SDL.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>

#include "./../../eggsfm/xfm_api.h"
#include "./../../eggsfm/xfm_wavplay.h"
#include "./../../eggsfm/xfm_export.h"
#include "./songs_data.h"

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./adaptive_audio.h"

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

static inline std::string AdaptiveAudio_RemapBuiltinMusicInstrumentIdsToHigh(const char *pattern)
{
    if (!pattern) return {};
    std::string out(pattern);

    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        return -1;
    };
    auto hexDigit = [](int v) -> char {
        v &= 15;
        return (char)(v < 10 ? ('0' + v) : ('A' + (v - 10)));
    };

    size_t i = 0;
    while (i < out.size() && (out[i] == ' ' || out[i] == '\t' || out[i] == '\n' || out[i] == '\r')) i++;
    while (i < out.size() && out[i] >= '0' && out[i] <= '9') i++;
    while (i < out.size() && out[i] != '\n') i++;
    if (i < out.size() && out[i] == '\n') i++;

    int columnPos = 0;
    for (; i < out.size(); i++)
    {
        char c = out[i];
        if (c == '\n' || c == '|')
        {
            columnPos = 0;
            continue;
        }
        if (columnPos == 3 && i + 1 < out.size())
        {
            int hi = hex(out[i]);
            int lo = hex(out[i + 1]);
            if (hi >= 0 && lo >= 0)
            {
                int legacyInst = (hi << 4) | lo;
                if (legacyInst >= 0x00 && legacyInst <= 0x13)
                {
                    int newInst = 0xFF - legacyInst;
                    out[i] = hexDigit(newInst >> 4);
                    out[i + 1] = hexDigit(newInst);
                }
            }
        }
        columnPos++;
    }
    return out;
}

void AdaptiveAudio_Init(AdaptiveAudioSystem* self, float fpsThreshold)
{
    self->state = ADAPTIVE_MONITORING;
    self->monitoringStartTime = 0.0f;
    self->monitoringDuration = 5.0f;  // Monitor for 5 seconds
    self->currentFps = 0.0f;
    self->fpsThreshold = fpsThreshold;
    self->measuredAvgFps = 0.0f;

    for (int i = 0; i < TRACKER_BUILTIN_SONG_COUNT; i++) {
        self->songBuffers[i] = NULL;
        self->songBufferSizes[i] = 0;
    }
    for (int i = 0; i < GameSoundSystem::SFX_COUNT; i++) {
        self->sfxBuffers[i] = NULL;
        self->sfxBufferSizes[i] = 0;
    }
    self->exportProgress = 0;
    self->exportTotal = TRACKER_BUILTIN_SONG_COUNT + GameSoundSystem::SFX_COUNT;
    self->exportCurrent = 0;
    self->exportStatus[0] = '\0';
    self->exportTotalSeconds = 0.0f;
    self->exportedSeconds = 0.0f;
    self->exportTotalSamples = 0;
    self->exportRenderedSamples = 0;
    self->fpsMessage[0] = '\0';

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

    printf("[AdaptiveAudio] Initialized, monitoring FPS (threshold: %.1f)\n", fpsThreshold);
}

void AdaptiveAudio_Update(AdaptiveAudioSystem* self, float deltaTime, float currentFps)
{
    if (self->state == ADAPTIVE_DISABLED || self->state == ADAPTIVE_RESTARTING) return;

    if (self->state == ADAPTIVE_MONITORING) {
        if (self->monitoringStartTime == 0.0f) {
            self->monitoringStartTime = (float)SDL_GetTicks64() / 1000.0f;
        }

        // Use the fpsCounter's fps value directly (already accumulated over 5 seconds)
        self->currentFps = currentFps;

        float elapsed = (float)SDL_GetTicks64() / 1000.0f - self->monitoringStartTime;

        // Only make decision after fpsCounter has had time to accumulate (monitoringDuration)
        // and fpsCounter has a valid reading (fps > 0)
        if (elapsed >= self->monitoringDuration && currentFps > 0.0f) {
            self->measuredAvgFps = currentFps;  // Use fpsCounter's accumulated value
            snprintf(self->fpsMessage, sizeof(self->fpsMessage), "Your FPS is %.1f, which is considered low.", currentFps);
            printf("[AdaptiveAudio] Monitoring complete. FPS: %.2f, Threshold: %.2f\n",
                   currentFps, self->fpsThreshold);

            if (currentFps < self->fpsThreshold) {
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
    for (int i = 0; i < TRACKER_BUILTIN_SONG_COUNT; i++) {
        if (self->songBuffers[i]) {
            free(self->songBuffers[i]);
            self->songBuffers[i] = NULL;
        }
    }
    for (int i = 0; i < GameSoundSystem::SFX_COUNT; i++) {
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
    self->exportTotalSamples = 0;
    self->exportRenderedSamples = 0;
    self->exportProgress = 0;
    self->exportCurrent = 0;
    self->exportStatus[0] = '\0';
    self->exportStep = EXPORT_STEP_IDLE;
}

void AdaptiveAudio_ResetExport(AdaptiveAudioSystem* self)
{
    if (!self) return;

    for (int i = 0; i < TRACKER_BUILTIN_SONG_COUNT; i++) {
        if (self->songBuffers[i]) {
            free(self->songBuffers[i]);
            self->songBuffers[i] = NULL;
        }
        self->songBufferSizes[i] = 0;
    }
    for (int i = 0; i < GameSoundSystem::SFX_COUNT; i++) {
        if (self->sfxBuffers[i]) {
            free(self->sfxBuffers[i]);
            self->sfxBuffers[i] = NULL;
        }
        self->sfxBufferSizes[i] = 0;
    }
    if (self->sfxModule) {
        xfm_module_destroy(self->sfxModule);
        self->sfxModule = NULL;
    }
    if (self->songModule) {
        xfm_module_destroy(self->songModule);
        self->songModule = NULL;
    }

    memset(&self->songExportState, 0, sizeof(xfm_export_song_state));
    memset(&self->sfxExportState, 0, sizeof(xfm_export_sfx_state));
    self->exportStep = EXPORT_STEP_IDLE;
    self->exportProgress = 0;
    self->exportCurrent = 0;
    self->exportTotal = TRACKER_BUILTIN_SONG_COUNT + GameSoundSystem::SFX_COUNT;
    self->exportTotalSeconds = 0.0f;
    self->exportedSeconds = 0.0f;
    self->exportTotalSamples = 0;
    self->exportRenderedSamples = 0;
    self->exportStatus[0] = '\0';
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
        self->exportTotal = TRACKER_BUILTIN_SONG_COUNT + GameSoundSystem::SFX_COUNT;
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
        for (int i = 0; i < TRACKER_BUILTIN_SONG_COUNT; i++) {
            int rows = 0;
            const char* p = BUILTIN_SONG_REGISTRY[i].pattern;
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            float songSec = (float)rows * (float)std::max(1, BUILTIN_SONG_REGISTRY[i].speed) /
                            (float)std::max(1, BUILTIN_SONG_REGISTRY[i].tickRate);
            totalSeconds += songSec;
            totalSamples += (int)(songSec * sampleRate);
        }
        const char* sfxPatternsInit[] = {
            SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
            SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT,
            SFX_PAT_COIN_PICKUP, SFX_PAT_STRIKE, SFX_PAT_SPARE, SFX_PAT_NEUTRAL_ROLL,
            SFX_PAT_BALL_ROLLING, SFX_PAT_NOS_LOOP, SFX_PAT_WIN, SFX_PAT_LOSE, SFX_PAT_BUY, SFX_PAT_TYPEWRITER
        };
        static_assert(
            sizeof(sfxPatternsInit) / sizeof(sfxPatternsInit[0]) == GameSoundSystem::SFX_COUNT,
            "SFX pattern init table must match SFX_COUNT"
        );
        const int sfxSpeedInit = 3;
        const int sfxTickRateInit = 60;
        for (int i = 0; i < GameSoundSystem::SFX_COUNT; i++) {
            int rows = 0;
            const char* p = sfxPatternsInit[i];
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
            while (*p >= '0' && *p <= '9') {
                rows = rows * 10 + (*p - '0');
                p++;
            }
            float sfxSec = (float)rows * sfxSpeedInit / sfxTickRateInit;
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

    // Declare pattern arrays once (used by macros). Built-in music instruments are
    // remapped to 0xEC..0xFF so custom instruments can start at 0x00.
    static std::string remappedSongs[TRACKER_BUILTIN_SONG_COUNT];
    static bool remappedSongsReady = false;
    if (!remappedSongsReady)
    {
        for (int i = 0; i < TRACKER_BUILTIN_SONG_COUNT; ++i)
            remappedSongs[i] = AdaptiveAudio_RemapBuiltinMusicInstrumentIdsToHigh(BUILTIN_SONG_REGISTRY[i].pattern);
        remappedSongsReady = true;
    }
    const char* sfxPatternsArr[] = {
        SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
        SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT,
        SFX_PAT_COIN_PICKUP, SFX_PAT_STRIKE, SFX_PAT_SPARE, SFX_PAT_NEUTRAL_ROLL,
        SFX_PAT_BALL_ROLLING, SFX_PAT_NOS_LOOP, SFX_PAT_WIN, SFX_PAT_LOSE, SFX_PAT_BUY, SFX_PAT_TYPEWRITER
    };
    int sfxIdsArr[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    static_assert(
        sizeof(sfxPatternsArr) / sizeof(sfxPatternsArr[0]) == GameSoundSystem::SFX_COUNT,
        "SFX pattern export table must match SFX_COUNT"
    );
    static_assert(
        sizeof(sfxIdsArr) / sizeof(sfxIdsArr[0]) == GameSoundSystem::SFX_COUNT,
        "SFX id export table must match SFX_COUNT"
    );

    // Helper: update unified progress bar
    #define UPDATE_PROGRESS \
        if (self->exportTotalSamples > 0) { \
            self->exportProgress = (int)(self->exportRenderedSamples * 100 / self->exportTotalSamples); \
        }

    // Helper macro for song export BEGIN phase
    // Creates a FRESH module for each song to avoid YM3438 state leakage
    // (phase, envelopes, LFO) - matches original fix from commit 845ff55
    #define SONG_BEGIN(songIdx) \
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching song %d/%d...", songIdx + 1, TRACKER_BUILTIN_SONG_COUNT); \
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
        xfm_patch_set(self->songModule, 0xFF, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xFE, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xFD, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xFC, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xFB, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xFA, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF9, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF8, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF7, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF6, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF5, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF4, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF3, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF2, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF1, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xF0, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xEF, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xEE, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xED, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438); \
        xfm_patch_set(self->songModule, 0xEC, &PATCH_13_ROLL, sizeof(PATCH_13_ROLL), XFM_CHIP_YM3438); \
        xfm_module_set_lfo(self->songModule, BUILTIN_SONG_REGISTRY[songIdx].lfoEnabled, BUILTIN_SONG_REGISTRY[songIdx].lfoFrequency); \
        xfm_song_declare(self->songModule, songIdx + 1, remappedSongs[songIdx].c_str(), BUILTIN_SONG_REGISTRY[songIdx].tickRate, BUILTIN_SONG_REGISTRY[songIdx].speed); \
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
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching song %d/%d... %d%%", songIdx + 1, TRACKER_BUILTIN_SONG_COUNT, self->exportProgress); \
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
            const char* p = BUILTIN_SONG_REGISTRY[songIdx].pattern; \
            int rows = 0; \
            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; \
            while (*p >= '0' && *p <= '9') { rows = rows * 10 + (*p - '0'); p++; } \
            float songSeconds = (float)rows * (float)std::max(1, BUILTIN_SONG_REGISTRY[songIdx].speed) / (float)std::max(1, BUILTIN_SONG_REGISTRY[songIdx].tickRate); \
            self->exportedSeconds += songSeconds; \
            self->exportRenderedSamples += self->songExportState.samples_rendered; \
            UPDATE_PROGRESS; \
        } else { \
            printf("[AdaptiveAudio] ERROR: Failed to export song %d\n", songIdx + 1); \
        } \
        xfm_module_destroy(self->songModule); \
        self->songModule = NULL;

    // Helper macro for SFX export BEGIN phase
    // Resets SFX module state before each SFX (matches original behavior)
    #define SFX_BEGIN(sfxIdx, sfxId) \
        snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching SFX %d/%d...", sfxIdx + 1, GameSoundSystem::SFX_COUNT); \
        self->exportCurrent = TRACKER_BUILTIN_SONG_COUNT + sfxIdx; \
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
        xfm_patch_set(self->sfxModule, 0x13, &PATCH_13_ROLL, sizeof(PATCH_13_ROLL), XFM_CHIP_YM3438); \
        xfm_patch_set(self->sfxModule, 0x17, &PATCH_17_NOS_PAD, sizeof(PATCH_17_NOS_PAD), XFM_CHIP_YM3438); \
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
            snprintf(self->exportStatus, sizeof(self->exportStatus), "Caching SFX %d/%d... %d%%", sfxIdx + 1, GameSoundSystem::SFX_COUNT, self->exportProgress); \
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
        }

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
            self->currentSongIndex = 0;
            self->currentSfxIndex = 0;
            self->exportStep = EXPORT_STEP_SONG_BEGIN;
            break;
        }

        // Each song gets a fresh module to avoid YM3438 state leakage.
        case EXPORT_STEP_SONG_BEGIN:
            if (self->currentSongIndex >= TRACKER_BUILTIN_SONG_COUNT)
            {
                self->exportStep = EXPORT_STEP_SFX_BEGIN;
                break;
            }
            SONG_BEGIN(self->currentSongIndex)

        case EXPORT_STEP_SONG_STEP:
            SONG_STEP(self->currentSongIndex)

        case EXPORT_STEP_SONG_FINALIZE:
            SONG_FINALIZE(self->currentSongIndex)
            self->currentSongIndex++;
            self->exportStep = EXPORT_STEP_SONG_BEGIN;
            break;

        case EXPORT_STEP_SFX_BEGIN:
            if (self->currentSfxIndex >= GameSoundSystem::SFX_COUNT)
            {
                self->exportStep = EXPORT_STEP_CLEANUP;
                break;
            }
            SFX_BEGIN(self->currentSfxIndex, sfxIdsArr[self->currentSfxIndex])

        case EXPORT_STEP_SFX_STEP:
            SFX_STEP(self->currentSfxIndex)

        case EXPORT_STEP_SFX_FINALIZE:
            SFX_FINALIZE(self->currentSfxIndex)
            self->currentSfxIndex++;
            self->exportStep = EXPORT_STEP_SFX_BEGIN;
            break;

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
