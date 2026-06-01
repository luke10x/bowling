#pragma once

#include <SDL.h>
#include <string>
#include "./../../eggsfm/xfm_api.h"
#include "./../../eggsfm/xfm_wavplay.h"
#include "./../../eggsfm/xfm_export.h"

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./songs_data.h"
#include "../tracker/tracker_song_io.h"

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
        QUALITY_OFF = 2,     // Audio disabled
    } quality;

    // UI state
    bool activated;


    // Labels for buttons
    char musicVolLabels[5][10];
    char sfxVolLabels[5][10];
    char qualityLabels[3][20];
    
    // Song names for display
    char songNames[TRACKER_MAX_SONG_COUNT + 1][32];  // Index 1-5 used, 0 unused
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
	        SFX_BALL_ROLLING,
	        SFX_WIN,
	        SFX_LOSE,
	        SFX_BUY,
	        SFX_TYPEWRITER,
	        SFX_COUNT,
	        SFX_TRACKER_PREVIEW = 250
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
    bool audioDisabled = false;
    bool browserAudioSuspended = false;
    bool browserAutoplayFixApplied = false;

    // Current song index (for switching between songs)
    int currentSongIndex = 1;
    bool userSongVisible = false;
    char userSongName[TRACKER_SONG_NAME_CAPACITY] = "Song 000000";
    char userSongPattern[TRACKER_USER_SONG_PATTERN_CAPACITY] = {};
    int musicLoopStartRow = 0;
    int musicLoopEndRow = -1;
    xfm_voice_id trackerPreviewVoice = FM_VOICE_INVALID;

    // TODO repetition
    void* runtimeSongBuffers[4] = {nullptr, nullptr, nullptr, nullptr};
    int runtimeSongSizes[4] = {0, 0, 0, 0};
	    void* runtimeSfxBuffers[SFX_COUNT] = {};
	    int runtimeSfxSizes[SFX_COUNT] = {};
	    bool hasRuntimeWavBuffers = false;

	    // Set runtime WAV buffers (from adaptive audio export)
	    void setRuntimeWavBuffers(void* songs[4], int songSizes[4], void* sfxs[SFX_COUNT], int sfxSizes[SFX_COUNT]);

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

    // Built-in songs keep their original pattern text (with legacy instrument ids),
    // but we remap those ids to live at the end of the 0..255 instrument bank so
    // user-created instruments can use small ids starting at 0x00.
    mutable std::string remappedBuiltinSongPatterns[TRACKER_BUILTIN_SONG_COUNT] = {};
    mutable bool remappedBuiltinSongPatternsReady = false;

    bool isRestartAllowed() const;
    bool updateRestart();
    void startRestart(const char* songPattern);
    const char* getSongPattern(int songIndex) const;
    const char* getSongName(int songIndex) const;
    int visibleSongCount() const;
    bool setUserSong(const char *displayName, const char *pattern);
    static void audio_callback(void* userdata, Uint8* stream, int len);
    bool initSoundSystem(const char* songPattern);
    bool reopenAudioDevice();
    void suspendForBrowser();
    void resumeFromBrowser(const char* songPattern);
    void playCurrentMusic(bool restart = false);
    void startMusicAtRow(int row);
    void stopMusic();
    void shutdown();
    bool restartSoundSystem();
    void nextSong();
    void previousSong();
    void setMusicLoopRange(int startRow, int endRow);
    void clearMusicLoopRange();
    xfm_voice_id playSfx(int id, int priority);
    xfm_voice_id previewTrackerNote(
        int note,
        int octave,
        int instrument,
        int volume,
        const xfm_patch_opn *patchOverride = nullptr,
        const XfmMacro *macros = nullptr,
        const bool *macroEnabled = nullptr,
        const bool *macroValid = nullptr,
        bool held = false
    );
    void releaseTrackerPreviewNote();
    void stopSfx(xfm_voice_id voice);
    void stopAllSfx();
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
	    xfm_voice_id playSfxBallRolling();
	    void playSfxWin();
	    void playSfxLose();
	    void playSfxBuy();
	    void playSfxTypewriter();
	    void setMusicVolume(float v);
	    void setSfxVolume(float v);
    void showSoundSettings();
    void hideSoundSettings();

    // Used by tracker to re-upload all custom patches/macros after a sound reinit/resume.
    // Setting this flag causes the next tracker tick to force-push all instruments.
    bool trackerNeedsFullPatchSync = false;
};

inline void initSoundSettings(SoundSettings* self, GameSoundSystem* soundSystem);
