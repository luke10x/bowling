#pragma once


#include <SDL.h>

#include "sounds.h"

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

    EXPORT_STEP_SFX_7_BEGIN,
    EXPORT_STEP_SFX_7_STEP,
    EXPORT_STEP_SFX_7_FINALIZE,

    EXPORT_STEP_SFX_8_BEGIN,
    EXPORT_STEP_SFX_8_STEP,
    EXPORT_STEP_SFX_8_FINALIZE,

    EXPORT_STEP_SFX_9_BEGIN,
    EXPORT_STEP_SFX_9_STEP,
    EXPORT_STEP_SFX_9_FINALIZE,

    EXPORT_STEP_SFX_10_BEGIN,
    EXPORT_STEP_SFX_10_STEP,
    EXPORT_STEP_SFX_10_FINALIZE,

    EXPORT_STEP_CLEANUP,
    EXPORT_STEP_DONE
};
struct AdaptiveAudioSystem {
    AdaptiveAudioState state;

    // FPS monitoring
    float monitoringStartTime;
    float monitoringDuration;  // How long to monitor (seconds)
    float currentFps;
    float fpsThreshold;  // Below this triggers modal
    float measuredAvgFps;  // Average FPS after monitoring period (for display in modal)

    // WAV export (yieldable state machine)
    AdaptiveAudioExportStep exportStep;  // Current step in export state machine
    int exportSampleRate;
    int bufferSize;
    xfm_module* sfxModule;  // Persistent SFX module across yield calls
    int currentSongIndex;   // Which song we're on (0-3)
    int currentSfxIndex;    // Which SFX we're on (0-9)
    xfm_module* songModule; // Temporary song module (destroyed after each song)

    // Yieldable export state for current song/SFX
    xfm_export_song_state songExportState;
    xfm_export_sfx_state sfxExportState;

    void* songBuffers[4];  // 4 songs (malloc'd WAV data)
    int songBufferSizes[4];
    void* sfxBuffers[12];  // 12 SFX (malloc'd WAV data)
    int sfxBufferSizes[12];
    int exportProgress;  // 0-100
    int exportTotal;
    int exportCurrent;
    char exportStatus[128];
    float exportTotalSeconds;   // Total expected duration in seconds
    float exportedSeconds;      // Duration exported so far in seconds
    int exportTotalSamples;     // Total samples across all songs/SFX (for unified progress)
    int exportRenderedSamples;  // Cumulative samples rendered across all songs/SFX
    char fpsMessage[128];       // Formatted FPS message for modal display

    // UI
    bool showModal;
    // Result
    bool useWavMode;
    bool audioDisabled;
    bool restartRequested;
    bool restartUseWav;
};

// Forward declarations
void AdaptiveAudio_Init(AdaptiveAudioSystem* self, float fpsThreshold);
void AdaptiveAudio_Update(AdaptiveAudioSystem* self, float deltaTime, float currentFps);
bool AdaptiveAudio_ProcessEvent(AdaptiveAudioSystem* self, SDL_Event event);
bool AdaptiveAudio_ExportWAV(AdaptiveAudioSystem* self, int sampleRate);
void AdaptiveAudio_Cleanup(AdaptiveAudioSystem* self);


void initSoundSettings(SoundSettings* self, GameSoundSystem* soundSystem);
void applySoundSettings(SoundSettings* self);
bool processSoundSettingsEvent(SoundSettings* self, SDL_Event event);
void buildSoundSettingsClay(SoundSettings* self);
// #include "adaptive_audio.cpp"
