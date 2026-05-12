#pragma once

#include <SDL.h>
#include "./../../eggsfm/xfm_api.h"
#include "./../../eggsfm/xfm_wavplay.h"
#include "./../../eggsfm/xfm_export.h"

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./songs_data.h"

// -----------------------------------------------------------------------------
// Audio buffer size configuration
// -----------------------------------------------------------------------------

// Synth mode (OPN real-time synthesis) - always uses 256 samples
// This is optimal for low-latency real-time synthesis
//static const int SYNTH_BUFFER_SIZE = 1024;
static const int SYNTH_BUFFER_SIZE = 2048;

// WAV playback mode - configurable buffer size
// Larger values reduce CPU usage but increase latency
// Start with 1024 for testing, can be adjusted later
// static const int WAV_PLAYBACK_BUFFER_SIZE = 1024;
static const int WAV_PLAYBACK_BUFFER_SIZE = 2048;

// Forward declaration to break circular dependency with sounds.h
struct GameSoundSystem;

struct SoundSettings
{
    // Volume levels (0.0, 0.25, 0.5, 0.75, 1.0)
    float musicVolume;
    float sfxVolume;

    // Quality setting
    enum Quality {
        QUALITY_WAV = 0,      // WAV fallback
        // QUALITY_LOFI = 1,    // 11025 Hz realtime
        QUALITY_HIFI = 1,    // 44100 Hz realtime
    } quality;

    // UI state
    bool activated;


    // Labels for buttons
    char musicVolLabels[5][10];
    char sfxVolLabels[5][10];
    char qualityLabels[3][20];
    
    // Song names for display
    char songNames[5][32];  // Index 1-4 used, 0 unused
    char currentSongName[32];

    // Reference to sound system (not owned)
    GameSoundSystem* soundSystem;

    // Flag set when WAV quality is selected but buffers aren't loaded yet.
    // The game loop checks this and triggers adaptive audio export.
    bool needsWavExport;
    // Flag set while WAV export is in progress (for UI loading indicator)
    bool wavExportInProgress;
    char wavExportStatus[128];  // Status message like "Exporting song 2/4..."
};

struct GameSoundSystem
{
	    enum SfxId
	    {
	        SFX_BALL_HIT_LANE = 0,
	        SFX_BALL_HIT_PINS,
	        SFX_PIN_HIT_PIN,
	        SFX_SCORE_DISPLAY,
	        SFX_GUTTER,
	        SFX_TIMEOUT,
	        SFX_COIN_PICKUP,
	        SFX_STRIKE,
	        SFX_SPARE,
	        SFX_NEUTRAL_ROLL,
	        SFX_WIN,
	        SFX_LOSE,
	        SFX_BUY,
	        SFX_TYPEWRITER
	    };

    xfm_module* musicModule = nullptr;
    xfm_module* sfxModule   = nullptr;
    xfm_wav_module* wavMusicModule = nullptr;
    xfm_wav_module* wavSfxModule   = nullptr;

    SDL_AudioDeviceID audioDev = 0;
    std::atomic<bool> audioShutdownInProgress{false};  // Emscripten: atomic for cross-thread visibility

    float musicVolume = 0.5f;
    float sfxVolume   = 1.0f;
    int sampleRate = 44100;
    int obtainedSampleRate = 0;  // Actual sample rate from SDL
    bool useWavPlayback = false;  // Default to OPN synth mode (WAVs exported at runtime if needed)

    // Current song index (for switching between songs)
    int currentSongIndex = 1;

    // TODO repetition
    void* runtimeSongBuffers[4] = {nullptr, nullptr, nullptr, nullptr};
    int runtimeSongSizes[4] = {0, 0, 0, 0};
	    void* runtimeSfxBuffers[14] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	    int runtimeSfxSizes[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	    bool hasRuntimeWavBuffers = false;

	    // Set runtime WAV buffers (from adaptive audio export)
	    void setRuntimeWavBuffers(void* songs[4], int songSizes[4], void* sfxs[14], int sfxSizes[14]);

    // Sound settings UI - recurse
    SoundSettings settings;

    enum class RestartState {
        RESTART_IDLE,
        RESTART_PAUSE_AUDIO,      // Step 1: Pause audio device
        RESTART_WAIT_CALLBACKS,   // Step 2: Wait for pending callbacks (frames)
        RESTART_DESTROY_MODULES,  // Step 3: Destroy old modules
        RESTART_WAIT_MORE,        // Step 4: Wait a bit more
        RESTART_INIT_NEW,         // Step 5: Initialize new system
        RESTART_COMPLETE          // Step 6: Done
    };
    RestartState restartState = RestartState::RESTART_IDLE;
    int restartWaitFrames = 0;    // Counter for waiting frames
    int restartTargetFrames = 10; // Wait ~10 frames (~167ms at 60fps)
    std::string restartSongPattern; // Store song pattern for re-init
    float restartProgress = 0.0f;  // 0.0 to 1.0 for UI progress bar
    // Grace period after shutdown (prevent restart too soon)
    uint32_t shutdownCompleteTime = 0;  // SDL_GetTicks64() when shutdown completed
    static const uint32_t GRACE_PERIOD_MS = 500;  // 0.5 second grace period

    bool isRestartAllowed() const;
    bool updateRestart();
    void startRestart(const char* songPattern);
    static void audio_callback(void* userdata, Uint8* stream, int len);
    bool initSoundSystem(const char* songPattern);
    void shutdown();
    bool restartSoundSystem();
    void nextSong();
    void previousSong();
    void playSfx(int id, int priority);
    void playSfxBallHitLane();
    void playSfxBallHitPins();
    void playSfxPinHitsAnotherPin();
    void playSfxFinalScoreDisplayed();
    void playSfxBallInGutter();
    void playSfxBallTimeout();
    void playSfxCoinPickup();
	    void playSfxStrike();
	    void playSfxSpare();
	    void playSfxNeutralRoll();
	    void playSfxWin();
	    void playSfxLose();
	    void playSfxBuy();
	    void playSfxTypewriter();
	    void setMusicVolume(float v);
	    void setSfxVolume(float v);
    void showSoundSettings();
    void hideSoundSettings();
};

inline void initSoundSettings(SoundSettings* self, GameSoundSystem* soundSystem);
