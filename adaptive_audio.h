#pragma once

#include <SDL.h>
#include <cstdio>
#include <cstdint>
#include <vector>

#include "./../eggsfm/xfm_api.h"
#include "./../eggsfm/xfm_wavplay.h"
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
    ADAPTIVE_GENERATING,    // Generating WAV caches
    ADAPTIVE_SYNTH,         // Using synth mode (good FPS)
    ADAPTIVE_WAV,           // Using WAV mode (generated caches)
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
    
    // WAV generation
    int16_t* songBuffers[4];  // 4 songs
    int songBufferSizes[4];
    int16_t* sfxBuffers[6];  // 6 SFX
    int sfxBufferSizes[6];
    int generationProgress;  // 0-100
    int generationTotal;
    int generationCurrent;
    int obtainedSampleRate;  // The actual obtained sample rate
    
    // UI
    bool showModal;
    Clayton_Click useSynthClick;
    Clayton_Click useWavClick;
    Clayton_Click disableAudioClick;
    
    // Result
    bool useWavMode;
    bool audioDisabled;
    bool wavGenerationRequested;  // Flag to signal WAV generation request
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
        self->songBuffers[i] = nullptr;
        self->songBufferSizes[i] = 0;
    }
    for (int i = 0; i < 6; i++) {
        self->sfxBuffers[i] = nullptr;
        self->sfxBufferSizes[i] = 0;
    }
    
    self->generationProgress = 0;
    self->generationTotal = 10;  // 4 songs + 6 SFX
    self->generationCurrent = 0;
    self->obtainedSampleRate = 44100;
    
    self->showModal = false;
    self->useWavMode = false;
    self->audioDisabled = false;
    
    initClaytonClick(&self->useSynthClick, "adaptiveUseSynth");
    initClaytonClick(&self->useWavClick, "adaptiveUseWav");
    initClaytonClick(&self->disableAudioClick, "adaptiveDisableAudio");
    
    self->wavGenerationRequested = false;
    
    printf("[AdaptiveAudio] Initialized, monitoring FPS (threshold: %.1f)\n", fpsThreshold);
}

void AdaptiveAudio_Update(AdaptiveAudioSystem* self, float deltaTime, float currentFps)
{
    if (self->state == ADAPTIVE_DISABLED) return;
    
    if (self->state == ADAPTIVE_GENERATING) {
        // Generation happens in a separate function
        return;
    }
    
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

// Simple WAV file writer (in-memory)
static bool WriteWavToBuffer(int16_t* samples, int num_samples, int sample_rate, 
                              int16_t** outBuffer, int* outSize)
{
    if (!samples || num_samples <= 0) return false;
    
    // WAV file header size + data
    int headerSize = 44;
    int dataSize = num_samples * 2;  // 16-bit stereo
    int totalSize = headerSize + dataSize;
    
    *outBuffer = (int16_t*)malloc(totalSize);
    if (!*outBuffer) return false;
    
    *outSize = totalSize;
    uint8_t* ptr = (uint8_t*)*outBuffer;
    
    // RIFF header
    memcpy(ptr, "RIFF", 4); ptr += 4;
    uint32_t fileSize = dataSize + 36;
    memcpy(ptr, &fileSize, 4); ptr += 4;
    memcpy(ptr, "WAVE", 4); ptr += 4;
    
    // fmt chunk
    memcpy(ptr, "fmt ", 4); ptr += 4;
    uint32_t fmtChunkSize = 16;
    memcpy(ptr, &fmtChunkSize, 4); ptr += 4;
    uint16_t audioFormat = 1;  // PCM
    memcpy(ptr, &audioFormat, 2); ptr += 2;
    uint16_t numChannels = 2;  // Stereo
    memcpy(ptr, &numChannels, 2); ptr += 2;
    memcpy(ptr, &sample_rate, 4); ptr += 4;
    uint32_t byteRate = sample_rate * numChannels * 2;
    memcpy(ptr, &byteRate, 4); ptr += 4;
    uint16_t blockAlign = numChannels * 2;
    memcpy(ptr, &blockAlign, 2); ptr += 2;
    uint16_t bitsPerSample = 16;
    memcpy(ptr, &bitsPerSample, 2); ptr += 2;
    
    // data chunk
    memcpy(ptr, "data", 4); ptr += 4;
    memcpy(ptr, &dataSize, 4); ptr += 4;
    memcpy(ptr, samples, dataSize);
    
    return true;
}

// Render a song pattern to audio buffer
static std::vector<int16_t> RenderSongPattern(const char* songPattern, int sampleRate, 
                                               int bufferSize, int ticksPerStep, int songIndex)
{
    xfm_module* module = xfm_module_create(sampleRate, bufferSize, XFM_CHIP_YM3438);
    if (!module) {
        printf("Error: Failed to create xfm module for song %d\n", songIndex);
        return {};
    }
    
    // Load all patches (duplicated from sounds.h and game-wav-exporter.cpp)
    xfm_patch_set(module, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x09, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0A, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0B, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0C, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0D, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0E, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x10, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x11, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438);
    
    // Declare and play song
    xfm_song_declare(module, songIndex, songPattern, 60, ticksPerStep);
    xfm_song_play(module, songIndex, true);
    
    // Render for a fixed duration (e.g., 2 minutes = 120 seconds)
    int totalSamples = sampleRate * 120 * 2;  // Stereo
    std::vector<int16_t> buffer(totalSamples);
    
    int samplesRendered = 0;
    int chunkSize = 4096;
    
    while (samplesRendered < totalSamples) {
        int toRender = (chunkSize < totalSamples - samplesRendered) ? chunkSize : (totalSamples - samplesRendered);
        xfm_mix_song(module, &buffer[samplesRendered], toRender);
        samplesRendered += toRender * 2;  // Stereo
    }
    
    xfm_module_destroy(module);
    return buffer;
}

// Render SFX to audio buffer
static std::vector<int16_t> RenderSfxPattern(const char* sfxPattern, int sampleRate, 
                                              int bufferSize, int sfxIndex)
{
    xfm_module* module = xfm_module_create(sampleRate, bufferSize, XFM_CHIP_YM3438);
    if (!module) {
        printf("Error: Failed to create xfm module for SFX %d\n", sfxIndex);
        return {};
    }
    
    // Load SFX patches
    xfm_patch_set(module, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
    
    // Declare and play SFX
    xfm_sfx_declare(module, sfxIndex, sfxPattern, 60, 3);
    xfm_sfx_play(module, sfxIndex, 5);
    
    // Render for a short duration (e.g., 2 seconds)
    int totalSamples = sampleRate * 2 * 2;  // Stereo
    std::vector<int16_t> buffer(totalSamples);
    
    int samplesRendered = 0;
    int chunkSize = 1024;
    
    while (samplesRendered < totalSamples) {
        int toRender = (chunkSize < totalSamples - samplesRendered) ? chunkSize : (totalSamples - samplesRendered);
        xfm_mix_sfx(module, &buffer[samplesRendered], toRender);
        samplesRendered += toRender * 2;  // Stereo
    }
    
    xfm_module_destroy(module);
    return buffer;
}

void AdaptiveAudio_GenerateWAV(AdaptiveAudioSystem* self, int sampleRate)
{
    printf("[AdaptiveAudio] Starting WAV generation at %d Hz...\n", sampleRate);
    self->state = ADAPTIVE_GENERATING;
    self->generationProgress = 0;
    self->generationCurrent = 0;
    self->generationTotal = 10;  // 4 songs + 6 SFX
    self->obtainedSampleRate = sampleRate;
    
    int bufferSize = 4096;
    
    // Generate song buffers
    const char* songPatterns[] = { SONG_01, SONG_02, SONG_03, SONG_04 };
    int songTicks[] = { 6, 8, 6, 6 };
    
    for (int i = 0; i < 4; i++) {
        printf("[AdaptiveAudio] Generating song %d/4...\n", i + 1);
        self->generationCurrent = i;
        self->generationProgress = (i * 100) / self->generationTotal;
        
        auto songData = RenderSongPattern(songPatterns[i], sampleRate, bufferSize, songTicks[i], i + 1);
        
        if (!songData.empty()) {
            self->songBufferSizes[i] = (int)songData.size() * 2;  // bytes
            self->songBuffers[i] = (int16_t*)malloc(self->songBufferSizes[i]);
            if (self->songBuffers[i]) {
                memcpy(self->songBuffers[i], songData.data(), self->songBufferSizes[i]);
            }
        }
    }
    
    // Generate SFX buffers
    const char* sfxPatterns[] = {
        SFX_PAT_BALL_HIT_LANE, SFX_PAT_BALL_HIT_PINS, SFX_PAT_PIN_HIT_PIN,
        SFX_PAT_SCORE_DISPLAY, SFX_PAT_GUTTER, SFX_PAT_TIMEOUT
    };
    
    for (int i = 0; i < 6; i++) {
        printf("[AdaptiveAudio] Generating SFX %d/6...\n", i + 1);
        self->generationCurrent = 4 + i;
        self->generationProgress = ((4 + i) * 100) / self->generationTotal;
        
        auto sfxData = RenderSfxPattern(sfxPatterns[i], sampleRate, bufferSize, i);
        
        if (!sfxData.empty()) {
            self->sfxBufferSizes[i] = (int)sfxData.size() * 2;  // bytes
            self->sfxBuffers[i] = (int16_t*)malloc(self->sfxBufferSizes[i]);
            if (self->sfxBuffers[i]) {
                memcpy(self->sfxBuffers[i], sfxData.data(), self->sfxBufferSizes[i]);
            }
        }
    }
    
    self->generationProgress = 100;
    self->state = ADAPTIVE_WAV;
    self->useWavMode = true;
    printf("[AdaptiveAudio] WAV generation complete!\n");
}

void AdaptiveAudio_Cleanup(AdaptiveAudioSystem* self)
{
    for (int i = 0; i < 4; i++) {
        if (self->songBuffers[i]) {
            free(self->songBuffers[i]);
            self->songBuffers[i] = nullptr;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (self->sfxBuffers[i]) {
            free(self->sfxBuffers[i]);
            self->sfxBuffers[i] = nullptr;
        }
    }
}

void AdaptiveAudio_RenderUI(AdaptiveAudioSystem* self)
{
    if (self->state != ADAPTIVE_DECIDING && self->state != ADAPTIVE_GENERATING) {
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
                CLAY_TEXT(CLAY_STRING("Synth: Real-time synthesis (lower CPU, may still be slow)"), 
                          CLAY_TEXT_CONFIG(bodyFontCfg));
                CLAY_TEXT(CLAY_STRING("WAV: Pre-rendered audio (higher memory, better performance)"), 
                          CLAY_TEXT_CONFIG(bodyFontCfg));
            } else if (self->state == ADAPTIVE_GENERATING) {
                // Show progress
                CLAY_TEXT(CLAY_STRING("🎵 Generating WAV Audio..."), CLAY_TEXT_CONFIG(titleFontCfg));
                
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
                    float progress = self->generationProgress / 100.0f;
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
                
                // Progress text
                char progressText[64];
                int len = snprintf(progressText, sizeof(progressText), "Progress: %d%% (%d/%d)", 
                                   self->generationProgress, self->generationCurrent, self->generationTotal);
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
        self->state = ADAPTIVE_SYNTH;
        self->useWavMode = false;
        self->showModal = false;
        printf("[AdaptiveAudio] User chose Synth mode\n");
        return true;
    }
    
    if (isClaytonClicked(&self->useWavClick, event)) {
        // Signal that WAV generation was requested
        self->wavGenerationRequested = true;
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
