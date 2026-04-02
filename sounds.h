#pragma once

#include <SDL.h>

#include "./../eggsfm/xfm_api.h"
#include "./../eggsfm/xfm_impl.cpp"
#include "./../eggsfm/xfm_wavplay.h"
#include "./../eggsfm/xfm_wavplay.cpp"
#include "./sounds/songs_data.h"
#include "./assets/sound_out/all_wav_xxd.h"

#include <cstdio>
#include <cstring>

#include <clay.h>
#include "./clayton/clayton_click.h"

// Forward declaration to break circular dependency with sounds.h
struct GameSoundSystem;

// -----------------------------------------------------------------------------
// Sound Settings Panel - Clay UI for audio configuration
// -----------------------------------------------------------------------------

struct SoundSettings
{
    // Volume levels (0.0, 0.25, 0.5, 0.75, 1.0)
    float musicVolume;
    float sfxVolume;

    // Quality setting
    enum Quality {
        QUALITY_WAV = 0,      // WAV fallback
        QUALITY_LOFI = 1,    // 11025 Hz realtime
        QUALITY_HIFI = 2,    // 44100 Hz realtime
    } quality;

    // UI state
    bool activated;

    // Click handlers
    Clayton_Click musicVolClicks[5];    // 5 volume buttons for music
    Clayton_Click sfxVolClicks[5];      // 5 volume buttons for SFX
    Clayton_Click qualityClicks[3];     // 3 quality buttons
    Clayton_Click nextSongClick;
    Clayton_Click closeClick;

    // Labels for buttons
    char musicVolLabels[5][10];
    char sfxVolLabels[5][10];
    char qualityLabels[3][20];

    // Reference to sound system (not owned)
    GameSoundSystem* soundSystem;
};

// -----------------------------------------------------------------------------
// Function declarations (implementations in sounds.h after GameSoundSystem is defined)
// -----------------------------------------------------------------------------

void initSoundSettings(SoundSettings* self, GameSoundSystem* soundSystem);
void applySoundSettings(SoundSettings* self);
bool processSoundSettingsEvent(SoundSettings* self, SDL_Event event);
void buildSoundSettingsClay(SoundSettings* self);

/* clang-format off */
// Patches are now defined in sounds/songs_data.h
/* clang-format on */

struct GameSoundSystem
{
    // ------------------------------------------------------------------------
    // SFX identifiers
    // ------------------------------------------------------------------------
    enum SfxId
    {
        SFX_BALL_HIT_LANE = 0,
        SFX_BALL_HIT_PINS,
        SFX_PIN_HIT_PIN,
        SFX_SCORE_DISPLAY,
        SFX_GUTTER,
        SFX_TIMEOUT
    };

    // ------------------------------------------------------------------------
    // Modules
    // ------------------------------------------------------------------------

    xfm_module* musicModule = nullptr;
    xfm_module* sfxModule   = nullptr;
    xfm_wav_module* wavMusicModule = nullptr;
    xfm_wav_module* wavSfxModule   = nullptr;

    SDL_AudioDeviceID audioDev = 0;

    float musicVolume = 0.5f;
    float sfxVolume   = 1.0f;
    int sampleRate = 44100;
    bool useWavPlayback = true;  // true = WAV mode, false = Synth mode (HiFi/LoFi)

    // Current song index (for switching between songs)
    int currentSongIndex = 1;

    // Sound settings UI
    SoundSettings settings;

    // ------------------------------------------------------------------------
    // Async restart state machine (for Emscripten)
    // ------------------------------------------------------------------------
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
    
    bool isRestartAllowed() const {
        if (restartState != RestartState::RESTART_IDLE && 
            restartState != RestartState::RESTART_COMPLETE) {
            return false;  // Restart in progress
        }
        // Check grace period
        if (shutdownCompleteTime > 0) {
            uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
            if (elapsed < GRACE_PERIOD_MS) {
                return false;  // Still in grace period
            }
        }
        return true;
    }

    // Call this every frame from game loop to progress restart state machine
    bool updateRestart()
    {
        if (restartState == RestartState::RESTART_IDLE ||
            restartState == RestartState::RESTART_COMPLETE) {
            return false;  // Nothing to do
        }

        switch (restartState) {
            case RestartState::RESTART_PAUSE_AUDIO:
                // Step 1: Pause audio device
                printf("[SoundRestart] Step 1/5: Pausing audio device...\n");
                if (audioDev) {
                    SDL_PauseAudioDevice(audioDev, 1);
                }
                restartState = RestartState::RESTART_WAIT_CALLBACKS;
                restartWaitFrames = restartTargetFrames;
                restartProgress = 0.2f;
                break;

            case RestartState::RESTART_WAIT_CALLBACKS:
                // Step 2: Wait for pending callbacks to finish
                restartWaitFrames--;
                if (restartWaitFrames <= 0) {
                    printf("[SoundRestart] Step 2/5: Callbacks finished, destroying modules...\n");
                    restartState = RestartState::RESTART_DESTROY_MODULES;
                    restartProgress = 0.4f;
                }
                break;

            case RestartState::RESTART_DESTROY_MODULES:
                // Step 3: Destroy old modules (callback now returns early)
                shutdown();
                shutdownCompleteTime = SDL_GetTicks64();  // Start grace period
                printf("[SoundRestart] Step 3/5: Modules destroyed, grace period started (%dms)\n", GRACE_PERIOD_MS);
                restartState = RestartState::RESTART_WAIT_MORE;
                restartWaitFrames = restartTargetFrames;
                restartProgress = 0.6f;
                break;

            case RestartState::RESTART_WAIT_MORE:
                // Step 4: Wait for grace period to complete before re-init
                restartWaitFrames--;
                {
                    uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
                    if (restartWaitFrames <= 0 && elapsed >= GRACE_PERIOD_MS) {
                        printf("[SoundRestart] Step 4/5: Grace period complete (%dms), re-initializing...\n", elapsed);
                        restartState = RestartState::RESTART_INIT_NEW;
                        restartProgress = 0.8f;
                    } else if (elapsed < GRACE_PERIOD_MS) {
                        // Still waiting for grace period
                        if (restartWaitFrames <= 0) {
                            restartWaitFrames = 1;  // Keep checking
                        }
                    }
                }
                break;

            case RestartState::RESTART_INIT_NEW:
                // Step 5: Initialize new system
                {
                    printf("[SoundRestart] Step 5/5: Loading %s...\n", 
                           !useWavPlayback ? (sampleRate == 44100 ? "HiFi 44100" : "LoFi 11025") : "WAV");
                    bool result = initSoundSystem(restartSongPattern.c_str());
                    restartState = result ? RestartState::RESTART_COMPLETE : RestartState::RESTART_IDLE;
                    restartProgress = result ? 1.0f : 0.0f;
                    if (result) {
                        printf("[SoundRestart] ✓ Restart complete - audio ready!\n");
                    } else {
                        printf("[SoundRestart] ✗ Restart FAILED!\n");
                    }
                }
                break;

            case RestartState::RESTART_COMPLETE:
                // Step 6: Done
                printf("[SoundRestart] Complete - resuming audio\n");
                restartState = RestartState::RESTART_IDLE;
                restartProgress = 0.0f;
                break;

            default:
                restartState = RestartState::RESTART_IDLE;
                break;
        }

        return true;  // Still in progress
    }

    // Start async restart - call this from applySoundSettings
    void startRestart(const char* songPattern)
    {
        if (!isRestartAllowed()) {
            if (restartState != RestartState::RESTART_IDLE && 
                restartState != RestartState::RESTART_COMPLETE) {
                printf("[SoundRestart] ERROR: Restart already in progress (state=%d), ignoring\n", (int)restartState);
            } else {
                uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
                printf("[SoundRestart] ERROR: Grace period not elapsed (%dms < %dms), ignoring\n", 
                       elapsed, GRACE_PERIOD_MS);
            }
            return;
        }
        restartSongPattern = songPattern;
        restartState = RestartState::RESTART_PAUSE_AUDIO;
        printf("[SoundRestart] Starting async restart (target frames per wait=%d)\n", restartTargetFrames);
    }

    // ------------------------------------------------------------------------
    // Audio callback (mix both modules)
    // ------------------------------------------------------------------------

    static void audio_callback(void* userdata, Uint8* stream, int len)
    {
        GameSoundSystem* self = (GameSoundSystem*)userdata;

        // If restarting, output silence (no modules should be active)
        if (self->restartState != RestartState::RESTART_IDLE &&
            self->restartState != RestartState::RESTART_COMPLETE) {
            std::memset(stream, 0, len);
            return;
        }

        // Safety check - if modules are null, just output silence
        if (!self->useWavPlayback) {
            if (!self->musicModule && !self->sfxModule) {
                std::memset(stream, 0, len);
                return;
            }
        } else {
            if (!self->wavMusicModule && !self->wavSfxModule) {
                std::memset(stream, 0, len);
                return;
            }
        }

        int16_t* out = (int16_t*)stream;

        // len is in bytes. For stereo 16-bit audio:
        // 1 sample = 2 bytes
        // 1 frame (L + R) = 4 bytes
        // So number of frames = total bytes / 4
        int frames = len / 4;

        // Clear output buffer
        std::memset(out, 0, len);

        // Mix music (song only - more efficient!)
        if (!self->useWavPlayback) {
            if (self->musicModule)
                xfm_mix_song(self->musicModule, out, frames);

            // Mix SFX into temp buffer then add (SFX only - more efficient!)
            if (self->sfxModule)
            {
                static int16_t sfxBuf[4096 * 2];  // enough for your buffer size

                std::memset(sfxBuf, 0, sizeof(sfxBuf));
                xfm_mix_sfx(self->sfxModule, sfxBuf, frames);

                for (int i = 0; i < frames * 2; i++)
                {
                    int32_t mixed = (int32_t)out[i] + sfxBuf[i];
                    if (mixed > 32767) mixed = 32767;
                    if (mixed < -32768) mixed = -32768;
                    out[i] = (int16_t)mixed;
                }
            }
        } else {
            if (self->wavMusicModule)
                xfm_wav_mix_song(self->wavMusicModule, out, frames);

            // Mix SFX into temp buffer then add (SFX only - more efficient!)
            if (self->wavSfxModule)
            {
                static int16_t sfxBuf[4096 * 2];  // enough for your buffer size

                std::memset(sfxBuf, 0, sizeof(sfxBuf));
                xfm_wav_mix_sfx(self->wavSfxModule, sfxBuf, frames);

                for (int i = 0; i < frames * 2; i++)
                {
                    int32_t mixed = (int32_t)out[i] + sfxBuf[i];
                    if (mixed > 32767) mixed = 32767;
                    if (mixed < -32768) mixed = -32768;
                    out[i] = (int16_t)mixed;
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // Init
    // ------------------------------------------------------------------------

    bool initSoundSystem(const char* songPattern)
    {
        printf("[SoundInit] Initializing in %s mode...\n", 
               !useWavPlayback ? (sampleRate == 44100 ? "HiFi SYNTH 44100" : "LoFi SYNTH 11025") : "WAV");
        
        SDL_AudioSpec desired{};
        // Use the sampleRate setting (44100 for HiFi, 11025 for LoFi)
        desired.freq     = useWavPlayback ? 11025 : sampleRate;
        desired.format   = AUDIO_S16SYS;
        desired.channels = 2;
        desired.samples  = 256;
        desired.callback = audio_callback;
        desired.userdata = this;

        SDL_AudioSpec obtained{};

        audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (!audioDev)
        {
            printf("Audio error: %s\n", SDL_GetError());
            return false;
        }

        // Safety check - obtained.freq must be valid
        if (obtained.freq <= 0) {
            printf("Audio error: invalid sample rate %d\n", obtained.freq);
            SDL_CloseAudioDevice(audioDev);
            audioDev = 0;
            return false;
        }

        printf("Audio: %d Hz, %d samples (%.1f ms latency)\n",
               obtained.freq, obtained.samples,
               obtained.samples * 1000.0 / obtained.freq);

        // Create modules with the obtained sample rate
        if (!this->useWavPlayback) {
            printf("[SoundInit] Creating SYNTH modules at %d Hz...\n", obtained.freq);
            musicModule = xfm_module_create(obtained.freq, obtained.samples, XFM_CHIP_YM3438);
            sfxModule   = xfm_module_create(obtained.freq, obtained.samples, XFM_CHIP_YM3438);
            wavMusicModule = nullptr;
            wavSfxModule = nullptr;
            if (!musicModule || !sfxModule)
            {
                printf("xfm_module_create failed\n");
                return false;
            }
            printf("[SoundInit] SYNTH modules created: music=%p, sfx=%p\n", (void*)musicModule, (void*)sfxModule);
        } else {
            printf("[SoundInit] Creating WAV modules at %d Hz...\n", obtained.freq);
            wavMusicModule = xfm_wav_module_create(obtained.freq, obtained.samples);
            wavSfxModule = xfm_wav_module_create(obtained.freq, obtained.samples);
            musicModule = nullptr;
            sfxModule = nullptr;
            if (!wavMusicModule || !wavSfxModule)
            {
                printf("xfm_wav_module_create failed\n");
                return false;
            }
            printf("[SoundInit] WAV modules created: music=%p, sfx=%p\n", (void*)wavMusicModule, (void*)wavSfxModule);
        }

        // --------------------------------------------------------------------
        // Load patches (use XFM_CHIP_YM3438 to match module creation)
        // --------------------------------------------------------------------

        if (!this->useWavPlayback) {

            // DUPLICATED logic in wav-exporter !

            // For song 1
            xfm_patch_set(musicModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);  // Hi-hat channel

            // For song 2 
            xfm_patch_set(musicModule, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);  // Hi-hat channel
            xfm_patch_set(musicModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);  // Hi-hat channel
            xfm_patch_set(musicModule, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);  // Hi-hat channel
            xfm_patch_set(musicModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);  // Hi-hat channel

            // FOR song 3
            xfm_patch_set(musicModule, 0x09, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x0A, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x0B, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x0C, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x0D, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438);

            // For song 4
            // Reuses OC (tick), 0d (snare)
            xfm_patch_set(musicModule, 0x0E, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x10, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438);
            xfm_patch_set(musicModule, 0x11, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438);

            // reuse for SFX
            xfm_patch_set(sfxModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
            xfm_patch_set(sfxModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
            xfm_patch_set(sfxModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
        }

        // --------------------------------------------------------------------
        // Declare song
        // --------------------------------------------------------------------

        if (!this->useWavPlayback) {
            printf("Declaring song...\n");
            xfm_song_declare(musicModule, 1, songPattern, 60, 6);
        } else {
            printf("Declaring WAV songs...\n");
            printf("  Loading song_01_xxd (%d bytes)...\n", song_01_xxd_len);
            xfm_wav_load_memory(wavMusicModule, XFM_WAV_SONG, 1, song_01_xxd, song_01_xxd_len, false);
            printf("  Loading song_02_xxd (%d bytes)...\n", song_02_xxd_len);
            xfm_wav_load_memory(wavMusicModule, XFM_WAV_SONG, 2, song_02_xxd, song_02_xxd_len, false);
            printf("  Loading song_03_xxd (%d bytes)...\n", song_03_xxd_len);
            xfm_wav_load_memory(wavMusicModule, XFM_WAV_SONG, 3, song_03_xxd, song_03_xxd_len, false);
            printf("  Loading song_04_xxd (%d bytes)...\n", song_04_xxd_len);
            xfm_wav_load_memory(wavMusicModule, XFM_WAV_SONG, 4, song_04_xxd, song_04_xxd_len, false);
            printf("  Loading WAV SFX...\n");
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_BALL_HIT_LANE, sfx_ball_hit_lane_xxd, sfx_ball_hit_lane_xxd_len, false);
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_BALL_HIT_PINS, sfx_ball_hit_pins_xxd, sfx_ball_hit_pins_xxd_len, false);
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_PIN_HIT_PIN, sfx_pin_hit_pin_xxd, sfx_pin_hit_pin_xxd_len, false);
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_SCORE_DISPLAY, sfx_score_display_xxd, sfx_score_display_xxd_len, false);
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_GUTTER, sfx_gutter_xxd, sfx_gutter_xxd_len, false);
            xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, SFX_TIMEOUT, sfx_timeout_xxd, sfx_timeout_xxd_len, false);
            printf("  WAV SFX loaded\n");
        }

        // --------------------------------------------------------------------
        // Declare SFX (patterns now use instrument 00)
        // --------------------------------------------------------------------

        if (!this->useWavPlayback) {
            xfm_sfx_declare(sfxModule, SFX_BALL_HIT_LANE,   SFX_PAT_BALL_HIT_LANE,   60, 3);
            xfm_sfx_declare(sfxModule, SFX_BALL_HIT_PINS,   SFX_PAT_BALL_HIT_PINS,   60, 3);
            xfm_sfx_declare(sfxModule, SFX_PIN_HIT_PIN,     SFX_PAT_PIN_HIT_PIN,     60, 3);
            xfm_sfx_declare(sfxModule, SFX_SCORE_DISPLAY,   SFX_PAT_SCORE_DISPLAY,   60, 3);
            xfm_sfx_declare(sfxModule, SFX_GUTTER,          SFX_PAT_GUTTER,          60, 3);
            xfm_sfx_declare(sfxModule, SFX_TIMEOUT,         SFX_PAT_TIMEOUT,         60, 3);
        }
        // WAV SFX already loaded above with the songs
        // --------------------------------------------------------------------
        // Volume - apply stored volume levels (preserved across quality changes)
        // --------------------------------------------------------------------

        if (!this->useWavPlayback) {
            xfm_module_set_volume(musicModule, musicVolume);
            xfm_module_set_volume(sfxModule, sfxVolume);
            printf("[SoundInit] Synth volumes set: music=%.2f, sfx=%.2f\n", musicVolume, sfxVolume);
        } else {
            xfm_wav_module_set_volume(wavMusicModule, musicVolume);
            xfm_wav_module_set_volume(wavSfxModule, sfxVolume);
            printf("[SoundInit] WAV volumes set: music=%.2f, sfx=%.2f\n", musicVolume, sfxVolume);
        }

        if (!this->useWavPlayback) {
            printf("Playing song...\n");
            xfm_song_play(musicModule, 1, true);
            printf("Music should be playing!\n");

            // Initialize sound settings UI
            initSoundSettings(&settings, this);
        } else {
            printf("Playing WAV song %d...\n", currentSongIndex);
            printf("  wavMusicModule=%p\n", (void*)wavMusicModule);
            xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
            printf("  xfm_wav_song_play returned\n");

            // Initialize sound settings UI
            initSoundSettings(&settings, this);
        }

        SDL_PauseAudioDevice(audioDev, 0);
        printf("DEBUG: useWavPlayback=%d, musicModule=%p, wavMusicModule=%p\n",
        useWavPlayback, (void*)musicModule, (void*)wavMusicModule);

        return true;
    }

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

    void shutdown()
    {
        printf("[SoundShutdown] Shutting down audio (useWavPlayback=%d)...\n", useWavPlayback);
        
        // Pause audio first
        if (audioDev) {
            SDL_PauseAudioDevice(audioDev, 1);
        }

        if (audioDev)
        {
            SDL_CloseAudioDevice(audioDev);
            audioDev = 0;
        }

        // Destroy synth modules
        if (musicModule)
        {
            printf("[SoundShutdown] Destroying musicModule %p\n", (void*)musicModule);
            xfm_module_destroy(musicModule);
            musicModule = nullptr;
        }

        if (sfxModule)
        {
            printf("[SoundShutdown] Destroying sfxModule %p\n", (void*)sfxModule);
            xfm_module_destroy(sfxModule);
            sfxModule = nullptr;
        }

        // Destroy WAV modules
        if (wavMusicModule)
        {
            printf("[SoundShutdown] Destroying wavMusicModule %p\n", (void*)wavMusicModule);
            xfm_wav_module_destroy(wavMusicModule);
            wavMusicModule = nullptr;
        }

        if (wavSfxModule)
        {
            printf("[SoundShutdown] Destroying wavSfxModule %p\n", (void*)wavSfxModule);
            xfm_wav_module_destroy(wavSfxModule);
            wavSfxModule = nullptr;
        }
        
        printf("[SoundShutdown] Complete\n");
    }

    // ------------------------------------------------------------------------
    // Restart sound system (async - for Emscripten)
    // Call startRestart() to begin, then call updateRestart() each frame
    // ------------------------------------------------------------------------

    bool restartSoundSystem()
    {
        // For async restart, just start the state machine
        const char* songPattern = nullptr;
        switch (currentSongIndex) {
            case 1: songPattern = SONG_01; break;
            case 2: songPattern = SONG_02; break;
            case 3: songPattern = SONG_03; break;
            case 4: songPattern = SONG_04; break;
            default: songPattern = SONG_01; break;
        }
        
        startRestart(songPattern);
        return true;  // Restart initiated (will complete asynchronously)
    }

    // ------------------------------------------------------------------------
    // Next song
    // ------------------------------------------------------------------------

    void nextSong()
    {
        // Cycle through songs 1 -> 2 -> 3 -> 4 -> 1
        currentSongIndex = (currentSongIndex % 4) + 1;

        const char* songPattern = nullptr;
        switch (currentSongIndex) {
            case 1: songPattern = SONG_01; break;
            case 2: songPattern = SONG_02; break;
            case 3: songPattern = SONG_03; break;
            case 4: songPattern = SONG_04; break;
        }

        int songTicksPerStep = 6;
        switch (currentSongIndex) {
            case 1: songTicksPerStep = 6; break;
            case 2: songTicksPerStep = 8; break;
            case 3: songTicksPerStep = 6; break;
            case 4: songTicksPerStep = 6; break;
        }

        if (musicModule && songPattern) {
            // Declare and play new song (this replaces the current one)
            xfm_song_declare(musicModule, currentSongIndex, songPattern, 60, songTicksPerStep);
            xfm_song_play(musicModule, currentSongIndex, true);
            printf("Playing song %d\n", currentSongIndex);
        }
        if (wavMusicModule) {
            xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
            printf("Playing WAW song %d\n", currentSongIndex);
        }
    }

    // ------------------------------------------------------------------------
    // SFX playback
    // ------------------------------------------------------------------------

    void playSfx(int id, int priority)
    {
        if (wavSfxModule) {
            SDL_LockAudioDevice(audioDev);
            xfm_wav_sfx_play(wavSfxModule, id, priority);
            SDL_UnlockAudioDevice(audioDev);
        }
        if (!sfxModule) return;

        SDL_LockAudioDevice(audioDev);
        xfm_sfx_play(sfxModule, id, priority);
        SDL_UnlockAudioDevice(audioDev);
    }

    // ------------------------------------------------------------------------
    // Game hooks
    // ------------------------------------------------------------------------

    void playSfxBallHitLane()        { playSfx(SFX_BALL_HIT_LANE, 3); }
    void playSfxBallHitPins()        { playSfx(SFX_BALL_HIT_PINS, 5); }
    void playSfxPinHitsAnotherPin()  { playSfx(SFX_PIN_HIT_PIN, 3); }
    void playSfxFinalScoreDisplayed(){ playSfx(SFX_SCORE_DISPLAY, 6); }
    void playSfxBallInGutter()       { playSfx(SFX_GUTTER, 5); }
    void playSfxBallTimeout()        { playSfx(SFX_TIMEOUT, 4); }

    // ------------------------------------------------------------------------
    // Volume
    // ------------------------------------------------------------------------

    void setMusicVolume(float v)
    {
        musicVolume = v;
        if (musicModule) xfm_module_set_volume(musicModule, v);
    }

    void setSfxVolume(float v)
    {
        sfxVolume = v;
        if (sfxModule) xfm_module_set_volume(sfxModule, v);
    }
    
    // ------------------------------------------------------------------------
    // Sound Settings UI
    // ------------------------------------------------------------------------
    
    void showSoundSettings()
    {
        settings.activated = true;
    }
    
    void hideSoundSettings()
    {
        settings.activated = false;
    }
    
    // bool processSoundSettingsEvent(SDL_Event event)
    // {
    //     return ::processSoundSettingsEvent(&settings, event);
    // }

    // void buildSoundSettingsClay()
    // {
    //     buildSoundSettingsClay(&settings);
    // }
};

// -----------------------------------------------------------------------------
// SoundSettings function implementations (must be after GameSoundSystem is defined)
// -----------------------------------------------------------------------------

inline void initSoundSettings(SoundSettings* self, GameSoundSystem* soundSystem)
{
    self->soundSystem = soundSystem;
    self->activated = false;

    // Initialize from sound system - read ACTUAL current values
    self->musicVolume = soundSystem->musicVolume;
    self->sfxVolume = soundSystem->sfxVolume;
    
    // Determine current quality mode from sound system state
    if (soundSystem->useWavPlayback) {
        self->quality = SoundSettings::QUALITY_WAV;
    } else if (soundSystem->sampleRate == 11025) {
        self->quality = SoundSettings::QUALITY_LOFI;
    } else {
        self->quality = SoundSettings::QUALITY_HIFI;
    }
    
    printf("[SoundSettings] Initialized: musicVol=%.2f, sfxVol=%.2f, quality=%d\n",
           self->musicVolume, self->sfxVolume, (int)self->quality);

    // Volume labels
    strcpy(self->musicVolLabels[0], "0%");
    strcpy(self->musicVolLabels[1], "25%");
    strcpy(self->musicVolLabels[2], "50%");
    strcpy(self->musicVolLabels[3], "75%");
    strcpy(self->musicVolLabels[4], "100%");

    memcpy(self->sfxVolLabels, self->musicVolLabels, sizeof(self->sfxVolLabels));

    // Quality labels
    strcpy(self->qualityLabels[0], "WAV");
    strcpy(self->qualityLabels[1], "LoFi 11025");
    strcpy(self->qualityLabels[2], "HiFi 44100");

    // Initialize clicks
    const char* volIds[] = { "musicVol0", "musicVol1", "musicVol2", "musicVol3", "musicVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&self->musicVolClicks[i], volIds[i]);
    }

    const char* sfxIds[] = { "sfxVol0", "sfxVol1", "sfxVol2", "sfxVol3", "sfxVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&self->sfxVolClicks[i], sfxIds[i]);
    }

    const char* qualIds[] = { "qualWav", "qualLofi", "qualHifi", };
    for (int i = 0; i < 3; i++) {
        initClaytonClick(&self->qualityClicks[i], qualIds[i]);
    }

    initClaytonClick(&self->nextSongClick, "nextSongClick");
    initClaytonClick(&self->closeClick, "soundSettingsClose");
}

inline void applySoundSettings(SoundSettings* self)
{
    if (!self->soundSystem) return;

    // Apply volume to modules immediately (no restart needed)
    // Volume changes do NOT affect quality setting
    if (self->soundSystem->musicModule) {
        xfm_module_set_volume(self->soundSystem->musicModule, self->musicVolume);
    }
    if (self->soundSystem->sfxModule) {
        xfm_module_set_volume(self->soundSystem->sfxModule, self->sfxVolume);
    }
    // WAV volume control
    if (self->soundSystem->wavMusicModule) {
        printf("[SoundVolume] WAV music volume: %.2f\n", self->musicVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavMusicModule, self->musicVolume);
    }
    if (self->soundSystem->wavSfxModule) {
        printf("[SoundVolume] WAV SFX volume: %.2f\n", self->sfxVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavSfxModule, self->sfxVolume);
    }

    // Check current mode BEFORE applying new setting
    bool wasWav = self->soundSystem->useWavPlayback;
    int wasSampleRate = self->soundSystem->sampleRate;
    
    // Apply new quality setting
    bool wantsWav = false;
    int wantsSampleRate = 44100;
    
    switch (self->quality) {
        case SoundSettings::QUALITY_HIFI:
            wantsWav = false;
            wantsSampleRate = 44100;
            self->soundSystem->sampleRate = 44100;
            printf("[SoundSettings] Quality requested: HiFi 44100 (synth)\n");
            break;
        case SoundSettings::QUALITY_LOFI:
            wantsWav = false;
            wantsSampleRate = 11025;
            self->soundSystem->sampleRate = 11025;
            printf("[SoundSettings] Quality requested: LoFi 11025 (synth)\n");
            break;
        case SoundSettings::QUALITY_WAV:
            wantsWav = true;
            wantsSampleRate = 11025;  // WAV always uses 44100
            self->soundSystem->sampleRate = 11025;
            printf("[SoundSettings] Quality requested: WAV (pre-rendered)\n");
            break;
    }
    
    // Check if mode actually changed (WAV flag OR sample rate)
    bool modeChanged = (wantsWav != wasWav) || (wantsSampleRate != wasSampleRate);
    
    if (modeChanged) {
        printf("[SoundSettings] Mode CHANGED (WAV=%d→%d, Rate=%d→%d) - scheduling restart...\n",
               wasWav, wantsWav, wasSampleRate, wantsSampleRate);
        
        // Apply new mode immediately (will take effect after restart)
        self->soundSystem->useWavPlayback = wantsWav;
        
        // Get current song pattern for restart
        const char* songPattern = SONG_01;
        switch (self->soundSystem->currentSongIndex) {
            case 1: songPattern = SONG_01; break;
            case 2: songPattern = SONG_02; break;
            case 3: songPattern = SONG_03; break;
            case 4: songPattern = SONG_04; break;
        }
        
        self->soundSystem->startRestart(songPattern);
    } else {
        printf("[SoundSettings] Mode unchanged (no restart needed)\n");
    }
}

inline bool processSoundSettingsEvent(SoundSettings* self, SDL_Event event)
{
    if (!self->activated) {
        return false;
    }

    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;

    if (!mouseDown && !mouseUp) {
        return false;
    }

    bool handled = false;

    // Music volume buttons
    for (int i = 0; i < 5; i++) {
        if (isClaytonClicked(&self->musicVolClicks[i], event)) {
            self->musicVolume = i * 0.25f;
            applySoundSettings(self);
            handled = true;
        }
    }

    // SFX volume buttons
    for (int i = 0; i < 5; i++) {
        if (isClaytonClicked(&self->sfxVolClicks[i], event)) {
            self->sfxVolume = i * 0.25f;
            applySoundSettings(self);
            handled = true;
        }
    }

    // Quality buttons
    for (int i = 0; i < 3; i++) {
        if (isClaytonClicked(&self->qualityClicks[i], event)) {
            self->quality = (SoundSettings::Quality)i;
            applySoundSettings(self);
            handled = true;
        }
    }

    // Next song button
    if (isClaytonClicked(&self->nextSongClick, event)) {
        if (self->soundSystem) {
            self->soundSystem->nextSong();
        }
        handled = true;
    }

    // Close button
    if (isClaytonClicked(&self->closeClick, event)) {
        self->activated = false;
        return true;
    }

    // If pointer is over the panel, consume the event (even if not on a button)
    // This prevents click-through to the game
    if (Clay_PointerOver(CLAY_ID("SoundSettingsContainer"))) {
        return true;
    }

    return handled;
}

inline void buildSoundSettingsClay(SoundSettings* self)
{
    if (!self->activated) {
        return;
    }

    // Font configs
    Clay_TextElementConfig labelFontCfg = {
        .textColor = {225, 225, 225, 255},
        .fontId = 0,
        .fontSize = (uint16_t)20,
    };

    Clay_TextElementConfig buttonFontCfg = {
        .textColor = {255, 255, 255, 255},
        .fontId = 2,
        .fontSize = (uint16_t)28,
    };

    Clay_TextElementConfig titleFontCfg = {
        .textColor = {255, 255, 255, 255},
        .fontId = 2,
        .fontSize = (uint16_t)36,
    };

    // Main container
    CLAY(
        CLAY_ID("SoundSettingsContainer"),
        {
            .layout = {
                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                .padding = {0, 0, 0, 0},
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }
    ) {
        // Settings panel window
        CLAY(
            CLAY_ID("SoundSettingsWindow"),
            {
                .layout = {
                    .sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_FIT()},
                    .padding = {20, 20, 20, 20},
                    .childGap = 15,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = {40, 40, 60, 255},
                .cornerRadius = {15, 15, 15, 15},
            }
        ) {
            // Title bar
            CLAY(
                CLAY_ID("SoundSettingsTitle"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {0, 0, 10, 0},
                        .childGap = 10,
                        .childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }
            ) {
                CLAY_TEXT(CLAY_STRING("Sound Settings"), CLAY_TEXT_CONFIG(titleFontCfg));

                /* -------- DIVIDER -------- */
                CLAY(
                    CLAY_ID("SoundSettingsTitleDivider"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}},
                    }
                ){};

                // Close button (right side)
                CLAY(
                    self->closeClick.clayId,
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(50), CLAY_SIZING_FIXED(50)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = {200, 50, 50, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }

            // Quality Section OR Restart Progress (mutually exclusive)
            if (self->soundSystem && self->soundSystem->restartProgress > 0.0f && self->soundSystem->restartProgress < 1.0f) {
                // Show progress indicator instead of quality buttons during restart
                CLAY(
                    CLAY_ID("RestartProgressSection"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = {80, 60, 40, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    Clay_TextElementConfig progressFontCfg = {
                        .textColor = {255, 255, 100, 255},
                        .fontId = 0,
                        .fontSize = (uint16_t)18,
                    };
                    CLAY_TEXT(CLAY_STRING("Changing quality..."), CLAY_TEXT_CONFIG(progressFontCfg));

                    // Progress bar background
                    CLAY(
                        CLAY_ID("ProgressBarBg"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(20)},
                            },
                            .backgroundColor = {40, 40, 40, 255},
                            .cornerRadius = {5, 5, 5, 5},
                        }
                    ) {
                        // Progress bar fill
                        float progress = self->soundSystem->restartProgress;
                        Clay_Color progressColor;
                        if (progress < 0.5f) {
                            progressColor = {200, 200, 50, 255};  // Yellow
                        } else if (progress < 0.8f) {
                            progressColor = {200, 150, 50, 255};  // Orange
                        } else {
                            progressColor = {50, 200, 50, 255};   // Green
                        }
                        
                        CLAY(
                            CLAY_ID("ProgressBarFill"),
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                                },
                                .backgroundColor = progressColor,
                                .cornerRadius = {5, 5, 5, 5},
                            }
                        ) {};
                    }

                    // Progress percentage text
                    char progressText[20];
                    int progressLen = snprintf(progressText, sizeof(progressText), "%d%%", 
                                               (int)(self->soundSystem->restartProgress * 100));
                    Clay_String progressStr = {
                        .isStaticallyAllocated = false,
                        .length = progressLen,
                        .chars = progressText,
                    };
                    CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(progressFontCfg));
                }
            } else {
                // Show quality buttons when not restarting
                CLAY(
                    CLAY_ID("QualitySection"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = {60, 60, 80, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("Audio Quality"), CLAY_TEXT_CONFIG(labelFontCfg));

                    // Quality buttons row
                    CLAY(
                        CLAY_ID("QualityRow"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .childGap = 8,
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }
                    ) {
                        for (int i = 0; i < 3; i++) {
                            Clay_Color btnColor = (self->quality == i) ?
                                Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

                            CLAY(
                                self->qualityClicks[i].clayId,
                                {
                                    .layout = {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                    .backgroundColor = btnColor,
                                    .cornerRadius = {8, 8, 8, 8},
                                    .border = {
                                        .color = {150, 150, 200, 255},
                                        .width = CLAY_BORDER_ALL(2),
                                    },
                                }
                            ) {
                                Clay_String label = {
                                    .isStaticallyAllocated = false,
                                    .length = (int)strlen(self->qualityLabels[i]),
                                    .chars = self->qualityLabels[i],
                                };
                                CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
                            }
                        }
                    }
                }
            }

            // Music Volume Section
            CLAY(
                CLAY_ID("MusicVolSection"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            ) {
                CLAY_TEXT(CLAY_STRING("Music Volume"), CLAY_TEXT_CONFIG(labelFontCfg));

                // Volume buttons row
                CLAY(
                    CLAY_ID("MusicVolRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                ) {
                    // Find which button should be highlighted (closest to current volume)
                    int selectedButton = -1;
                    float minDiff = 1.0f;
                    for (int i = 0; i < 5; i++) {
                        float targetVol = i * 0.25f;
                        float diff = fabsf(self->musicVolume - targetVol);
                        if (diff < minDiff) {
                            minDiff = diff;
                            selectedButton = i;
                        }
                    }
                    
                    for (int i = 0; i < 5; i++) {
                        Clay_Color btnColor = (i == selectedButton) ?
                            Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

                        CLAY(
                            self->musicVolClicks[i].clayId,
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = btnColor,
                                .cornerRadius = {8, 8, 8, 8},
                                .border = {
                                    .color = {150, 150, 200, 255},
                                    .width = CLAY_BORDER_ALL(2),
                                },
                            }
                        ) {
                            Clay_String label = {
                                .isStaticallyAllocated = false,
                                .length = (int)strlen(self->musicVolLabels[i]),
                                .chars = self->musicVolLabels[i],
                            };
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
                        }
                    }
                }
            }

            // SFX Volume Section
            CLAY(
                CLAY_ID("SfxVolSection"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .padding = {10, 10, 10, 10},
                        .childGap = 10,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            ) {
                CLAY_TEXT(CLAY_STRING("SFX Volume"), CLAY_TEXT_CONFIG(labelFontCfg));

                // Volume buttons row
                CLAY(
                    CLAY_ID("SfxVolRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                ) {
                    // Find which button should be highlighted (closest to current volume)
                    int selectedButton = -1;
                    float minDiff = 1.0f;
                    for (int i = 0; i < 5; i++) {
                        float targetVol = i * 0.25f;
                        float diff = fabsf(self->sfxVolume - targetVol);
                        if (diff < minDiff) {
                            minDiff = diff;
                            selectedButton = i;
                        }
                    }
                    
                    for (int i = 0; i < 5; i++) {
                        Clay_Color btnColor = (i == selectedButton) ?
                            Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

                        CLAY(
                            self->sfxVolClicks[i].clayId,
                            {
                                .layout = {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                                .backgroundColor = btnColor,
                                .cornerRadius = {8, 8, 8, 8},
                                .border = {
                                    .color = {150, 150, 200, 255},
                                    .width = CLAY_BORDER_ALL(2),
                                },
                            }
                        ) {
                            Clay_String label = {
                                .isStaticallyAllocated = false,
                                .length = (int)strlen(self->sfxVolLabels[i]),
                                .chars = self->sfxVolLabels[i],
                            };
                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
                        }
                    }
                }
            }


            // Action buttons row
            CLAY(
                CLAY_ID("ActionRow"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                        .childGap = 10,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    },
                }
            ) {
                // Next Song button
                CLAY(
                    self->nextSongClick.clayId,
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        },
                        .backgroundColor = {50, 100, 200, 255},
                        .cornerRadius = {10, 10, 10, 10},
                        .border = {
                            .color = {150, 150, 200, 255},
                            .width = CLAY_BORDER_ALL(2),
                        },
                    }
                ) {
                    CLAY_TEXT(CLAY_STRING("Next Song"), CLAY_TEXT_CONFIG(buttonFontCfg));
                }
            }
        }
    }
}
