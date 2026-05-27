#include <SDL.h>

#include "./../../eggsfm/xfm_api.h"
#include "./../../eggsfm/xfm_wavplay.h"
#include "./../../eggsfm/xfm_export.h"

// #include "./assets/sound_out/all_wav_xxd.h"  // Disabled - WAVs now exported at runtime

#include <cstdio>
#include <cstring>
#include <atomic>
#include <algorithm>

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./sounds.h"

// Forward declaration to break circular dependency with sounds.h
// struct GameSoundSystem;

// -----------------------------------------------------------------------------
// Sound Settings Panel - Clay UI for audio configuration
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
// Function declarations (implementations in sounds.h after GameSoundSystem is defined)
// -----------------------------------------------------------------------------



// Render WAV export loading indicator (called from game loop during export)
void buildWavExportLoadingIndicator(SoundSettings* self, int exportProgress, float exportedSeconds, float exportTotalSeconds, int sampleRate);
/* clang-format off */
// Patches are now defined in sounds/songs_data.h
/* clang-format on */

    // Set runtime WAV buffers (from adaptive audio export)
void GameSoundSystem::setRuntimeWavBuffers(void* songs[4], int songSizes[4], void* sfxs[SFX_COUNT], int sfxSizes[SFX_COUNT]) {
    for (int i = 0; i < 4; i++) {
        runtimeSongBuffers[i] = songs[i];
        runtimeSongSizes[i] = songSizes[i];
    }
    for (int i = 0; i < SFX_COUNT; i++) {
        runtimeSfxBuffers[i] = sfxs[i];
        runtimeSfxSizes[i] = sfxSizes[i];
    }
    hasRuntimeWavBuffers = true;
    printf("[SoundSystem] Runtime WAV buffers set\n");
}
bool GameSoundSystem::isRestartAllowed() const {
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

const char* GameSoundSystem::getSongPattern(int songIndex) const
{
    switch (songIndex) {
        case 1: return SONG_01;
        case 2: return SONG_02;
        case 3: return SONG_03;
        case 4: return SONG_04;
        case TRACKER_USER_SONG_SLOT:
            return userSongVisible && userSongPattern[0] ? userSongPattern : SONG_01;
        default: return SONG_01;
    }
}

const char* GameSoundSystem::getSongName(int songIndex) const
{
    switch (songIndex) {
        case 1: return "Bowling Strike";
        case 2: return "Gutter Groove";
        case 3: return "Pin Crusher";
        case 4: return "Alley Cat";
        case TRACKER_USER_SONG_SLOT: return userSongVisible ? userSongName : "Song 000000";
        default: return "Bowling Strike";
    }
}

int GameSoundSystem::visibleSongCount() const
{
    return userSongVisible ? TRACKER_MAX_SONG_COUNT : TRACKER_BUILTIN_SONG_COUNT;
}

bool GameSoundSystem::setUserSong(const char *displayName, const char *pattern)
{
    if (!displayName || !displayName[0] || !pattern || !pattern[0]) return false;
    std::snprintf(userSongName, sizeof(userSongName), "%s", displayName);
    std::snprintf(userSongPattern, sizeof(userSongPattern), "%s", pattern);
    userSongVisible = true;
    std::snprintf(settings.songNames[TRACKER_USER_SONG_SLOT], sizeof(settings.songNames[TRACKER_USER_SONG_SLOT]), "5. %s", userSongName);
    return true;
}

    // Call this every frame from game loop to progress restart state machine
bool GameSoundSystem::updateRestart()
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
void GameSoundSystem::startRestart(const char* songPattern)
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

static void my_audio_callback(void* userdata, Uint8* stream, int len)
{
    if (userdata == nullptr) {
        // To awoid bad memory errors in emscripten
        return;
    }
    GameSoundSystem* self = (GameSoundSystem*)userdata;

    // Emscripten: callback runs async, must check ALL state flags FIRST
    if (self->audioShutdownInProgress.load()) {
        std::memset(stream, 0, len);
        return;
    }

    // If restarting, output silence (no modules should be active)
    if (self->restartState != GameSoundSystem::RestartState::RESTART_IDLE &&
        self->restartState != GameSoundSystem::RestartState::RESTART_COMPLETE) {
        std::memset(stream, 0, len);
        return;
    }

    // Safety check - if NO modules are valid, just output silence
    bool hasValidModules = false;
    if (!self->useWavPlayback) {
        if (self->musicModule || self->sfxModule) hasValidModules = true;
    } else {
        if (self->wavMusicModule || self->wavSfxModule) hasValidModules = true;
    }
    if (!hasValidModules) {
        std::memset(stream, 0, len);
        return;
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
            // CRITICAL: Cap frames to prevent buffer overflow.
            // SDL may request more frames than expected on some platforms.
            int mix_frames = frames;
            if (mix_frames > 4096) mix_frames = 4096;
            
            static int16_t sfxBuf[4096 * 2];
            std::memset(sfxBuf, 0, mix_frames * 2 * sizeof(int16_t));
            xfm_mix_sfx(self->sfxModule, sfxBuf, mix_frames);

            for (int i = 0; i < mix_frames * 2; i++)
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
            int mix_frames = frames;
            if (mix_frames > 4096) mix_frames = 4096;
            
            static int16_t sfxBuf[4096 * 2];
            std::memset(sfxBuf, 0, mix_frames * 2 * sizeof(int16_t));
            xfm_wav_mix_sfx(self->wavSfxModule, sfxBuf, mix_frames);

            for (int i = 0; i < mix_frames * 2; i++)
            {
                int32_t mixed = (int32_t)out[i] + sfxBuf[i];
                if (mixed > 32767) mixed = 32767;
                if (mixed < -32768) mixed = -32768;
                out[i] = (int16_t)mixed;
            }
        }
    }
}

bool GameSoundSystem::initSoundSystem(const char* songPattern)
{
    if (audioDisabled) {
        printf("[SoundInit] Audio disabled; skipping audio initialization\n");
        return true;
    }

    printf("[SoundInit] Initializing in %s mode...\n", 
            !useWavPlayback ? (sampleRate == 44100 ? "HiFi SYNTH 44100" : "LoFi SYNTH 11025") : "WAV");
    
    SDL_AudioSpec desired{};
    // Use the sampleRate setting for both modes to ensure consistency
    desired.freq     = sampleRate;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 2;
    // Use different buffer sizes for synth vs WAV playback
    // Synth mode needs low latency (256), WAV playback can use larger buffers
    desired.samples  = useWavPlayback ? WAV_PLAYBACK_BUFFER_SIZE : SYNTH_BUFFER_SIZE;
    desired.callback = my_audio_callback;
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
    
    // Store the obtained sample rate for later use
    obtainedSampleRate = obtained.freq;

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
        xfm_patch_set(sfxModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);  // Hi-hat channel
        xfm_patch_set(sfxModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x12, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x13, &PATCH_13_ROLL, sizeof(PATCH_13_ROLL), XFM_CHIP_YM3438);
        xfm_module_set_lfo(sfxModule, true, 5);
    }

    // --------------------------------------------------------------------
    // Declare song
    // --------------------------------------------------------------------

    if (!this->useWavPlayback) {
        printf("Declaring song...\n");
        int songTicksPerStep = currentSongIndex == 2 ? 8 : 6;
        xfm_song_declare(musicModule, currentSongIndex, songPattern, 60, songTicksPerStep);
        musicLoopStartRow = 0;
        musicLoopEndRow = xfm_song_get_total_rows(musicModule, currentSongIndex) - 1;
    } else {
        printf("Declaring WAV songs...\n");
        if (hasRuntimeWavBuffers) {
            // Use runtime-exported WAV buffers
            printf("  Using runtime WAV buffers\n");
            for (int i = 0; i < 4; i++) {
                if (runtimeSongBuffers[i] && runtimeSongSizes[i] > 0) {
                    printf("  Loading song %d from runtime buffer (%d bytes)\n", i + 1, runtimeSongSizes[i]);
                    xfm_wav_load_memory(wavMusicModule, XFM_WAV_SONG, i + 1, runtimeSongBuffers[i], runtimeSongSizes[i], false);
                } else {
                    printf("  WARNING: Song %d buffer is empty!\n", i + 1);
                }
            }
            for (int i = 0; i < SFX_COUNT; i++) {
                if (runtimeSfxBuffers[i] && runtimeSfxSizes[i] > 0) {
                    printf("  Loading SFX %d from runtime buffer (%d bytes)\n", i, runtimeSfxSizes[i]);
                    int result = xfm_wav_load_memory(wavSfxModule, XFM_WAV_SFX, i, runtimeSfxBuffers[i], runtimeSfxSizes[i], false);
                    if (result == 0) {
                        printf("    ✓ SFX %d loaded successfully\n", i);
                    } else {
                        printf("    ✗ ERROR: Failed to load SFX %d (result=%d)\n", i, result);
                    }
                } else {
                    printf("  WARNING: SFX %d buffer is empty!\n", i);
                }
            }
        } else {
            printf("  WARNING: No WAV buffers available, music will be silent\n");
        }
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
        xfm_sfx_declare(sfxModule, SFX_COIN_PICKUP,     SFX_PAT_COIN_PICKUP,     60, 3);
        xfm_sfx_declare(sfxModule, SFX_STRIKE,          SFX_PAT_STRIKE,          60, 3);
        xfm_sfx_declare(sfxModule, SFX_SPARE,           SFX_PAT_SPARE,           60, 3);
        xfm_sfx_declare(sfxModule, SFX_NEUTRAL_ROLL,    SFX_PAT_NEUTRAL_ROLL,    60, 3);
        xfm_sfx_declare(sfxModule, SFX_BALL_ROLLING,    SFX_PAT_BALL_ROLLING,    60, 3);
        xfm_sfx_declare(sfxModule, SFX_WIN,             SFX_PAT_WIN,             60, 3);
        xfm_sfx_declare(sfxModule, SFX_LOSE,            SFX_PAT_LOSE,            60, 3);
        xfm_sfx_declare(sfxModule, SFX_BUY,             SFX_PAT_BUY,             60, 3);
        xfm_sfx_declare(sfxModule, SFX_TYPEWRITER,      SFX_PAT_TYPEWRITER,      60, 3);
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
        xfm_song_play(musicModule, currentSongIndex, true);
        xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
        printf("Music should be playing!\n");

        // Initialize sound settings UI
        // initSoundSettings(&settings, this);
    } else {
        printf("Playing WAV song %d...\n", currentSongIndex);
        printf("  wavMusicModule=%p\n", (void*)wavMusicModule);
        xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
        printf("  xfm_wav_song_play returned\n");

        // Initialize sound settings UI
        // initSoundSettings(&settings, this);
    }

    // Emscripten: Clear shutdown flag BEFORE unpausing device so callback sees ready state
    audioShutdownInProgress.store(false);

    SDL_PauseAudioDevice(audioDev, 0);
    printf("DEBUG: useWavPlayback=%d, musicModule=%p, wavMusicModule=%p\n",
    useWavPlayback, (void*)musicModule, (void*)wavMusicModule);

    return true;
}

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

void GameSoundSystem::shutdown()
{
    printf("[SoundShutdown] Shutting down audio (useWavPlayback=%d)...\n", useWavPlayback);

    // CRITICAL: Set shutdown flag FIRST - callback checks this before anything else
    audioShutdownInProgress.store(true);

    // CRITICAL: Close audio device COMPLETELY to stop callback on Emscripten
    // SDL_PauseAudioDevice is NOT enough - callback keeps running async
    if (audioDev) {
        SDL_CloseAudioDevice(audioDev);
        audioDev = 0;
        printf("[SoundShutdown] Audio device closed\n");
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


bool GameSoundSystem::restartSoundSystem()
{
    // For async restart, just start the state machine
    const char* songPattern = getSongPattern(currentSongIndex);
    startRestart(songPattern);
    return true;  // Restart initiated (will complete asynchronously)
}

    // ------------------------------------------------------------------------
    // Next song
    // ------------------------------------------------------------------------

void GameSoundSystem::nextSong()
{
    int count = visibleSongCount();
    currentSongIndex = (currentSongIndex % count) + 1;

    const char* songPattern = getSongPattern(currentSongIndex);

    int songTicksPerStep = 6;
    switch (currentSongIndex) {
        case 2: songTicksPerStep = 8; break;
        default: songTicksPerStep = 6; break;
    }

    if (musicModule && songPattern) {
        // Declare and play new song (this replaces the current one)
        xfm_song_declare(musicModule, currentSongIndex, songPattern, 60, songTicksPerStep);
        xfm_song_play(musicModule, currentSongIndex, true);
        clearMusicLoopRange();
        printf("Playing song %d\n", currentSongIndex);
    }
    if (wavMusicModule) {
        xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
        printf("Playing WAW song %d\n", currentSongIndex);
    }
    
    // Update UI song name
    strcpy(settings.currentSongName, settings.songNames[currentSongIndex]);
}

    // ------------------------------------------------------------------------
    // Previous song
    // ------------------------------------------------------------------------

void GameSoundSystem::previousSong()
{
    int count = visibleSongCount();
    currentSongIndex = ((currentSongIndex - 2 + count) % count) + 1;

    const char* songPattern = getSongPattern(currentSongIndex);

    int songTicksPerStep = 6;
    switch (currentSongIndex) {
        case 2: songTicksPerStep = 8; break;
        default: songTicksPerStep = 6; break;
    }

    if (musicModule && songPattern) {
        // Declare and play new song (this replaces the current one)
        xfm_song_declare(musicModule, currentSongIndex, songPattern, 60, songTicksPerStep);
        xfm_song_play(musicModule, currentSongIndex, true);
        clearMusicLoopRange();
        printf("Playing song %d\n", currentSongIndex);
    }
    if (wavMusicModule) {
        xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
        printf("Playing WAV song %d\n", currentSongIndex);
    }
    
    // Update UI song name
    strcpy(settings.currentSongName, settings.songNames[currentSongIndex]);
}

void GameSoundSystem::setMusicLoopRange(int startRow, int endRow)
{
    musicLoopStartRow = std::max(0, std::min(startRow, endRow));
    musicLoopEndRow = std::max(startRow, endRow);
    if (audioDisabled || useWavPlayback || !musicModule) return;

    SDL_LockAudioDevice(audioDev);
    xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    SDL_UnlockAudioDevice(audioDev);
}

void GameSoundSystem::clearMusicLoopRange()
{
    musicLoopStartRow = 0;
    musicLoopEndRow = -1;
    if (audioDisabled || useWavPlayback || !musicModule) return;

    int rows = xfm_song_get_total_rows(musicModule, currentSongIndex);
    musicLoopEndRow = rows > 0 ? rows - 1 : -1;
    SDL_LockAudioDevice(audioDev);
    xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
    SDL_UnlockAudioDevice(audioDev);
}

    // ------------------------------------------------------------------------
    // SFX playback
    // ------------------------------------------------------------------------

xfm_voice_id GameSoundSystem::playSfx(int id, int priority)
{
    if (audioDisabled) return FM_VOICE_INVALID;

    if (useWavPlayback) {
        // WAV mode: only play on wavSfxModule
        if (!wavSfxModule) {
            printf("[SFX] WARNING: wavSfxModule is null, cannot play SFX %d\n", id);
            return FM_VOICE_INVALID;
        }
        SDL_LockAudioDevice(audioDev);
        xfm_voice_id voice = xfm_wav_sfx_play(wavSfxModule, id, priority);
        SDL_UnlockAudioDevice(audioDev);
        return voice;
    } else {
        // SYNTH mode: only play on sfxModule
        if (!sfxModule) {
            printf("[SFX] WARNING: sfxModule is null, cannot play SFX %d\n", id);
            return FM_VOICE_INVALID;
        }
        SDL_LockAudioDevice(audioDev);
        xfm_voice_id voice = xfm_sfx_play(sfxModule, id, priority);
        SDL_UnlockAudioDevice(audioDev);
        return voice;
    }
}

void GameSoundSystem::stopSfx(xfm_voice_id voice)
{
    if (voice == FM_VOICE_INVALID) return;

    if (useWavPlayback) {
        if (!wavSfxModule) return;
        SDL_LockAudioDevice(audioDev);
        xfm_wav_sfx_stop(wavSfxModule, voice);
        SDL_UnlockAudioDevice(audioDev);
    } else {
        if (!sfxModule) return;
        SDL_LockAudioDevice(audioDev);
        xfm_sfx_stop(sfxModule, voice);
        SDL_UnlockAudioDevice(audioDev);
    }
}

void GameSoundSystem::stopAllSfx()
{
    if (useWavPlayback) {
        if (!wavSfxModule) return;
        SDL_LockAudioDevice(audioDev);
        xfm_wav_sfx_stop_all(wavSfxModule);
        SDL_UnlockAudioDevice(audioDev);
    } else {
        if (!sfxModule) return;
        SDL_LockAudioDevice(audioDev);
        xfm_sfx_stop_all(sfxModule);
        SDL_UnlockAudioDevice(audioDev);
    }
}

    // ------------------------------------------------------------------------
    // Game hooks
    // ------------------------------------------------------------------------

void GameSoundSystem::playSfxBallHitLane()        { playSfx(SFX_BALL_HIT_LANE, 3); }
void GameSoundSystem::playSfxBallHitPins()        { playSfx(SFX_BALL_HIT_PINS, 5); }
void GameSoundSystem::playSfxPinHitsAnotherPin()  { playSfx(SFX_PIN_HIT_PIN, 3); }
void GameSoundSystem::playSfxFinalScoreDisplayed(){ playSfx(SFX_SCORE_DISPLAY, 6); }
void GameSoundSystem::playSfxBallInGutter()       { playSfx(SFX_GUTTER, 5); }
void GameSoundSystem::playSfxBallTimeout()        { playSfx(SFX_TIMEOUT, 4); }
void GameSoundSystem::playSfxCoinPickup()         { playSfx(SFX_COIN_PICKUP, 4); }
void GameSoundSystem::playSfxStrike()             { playSfx(SFX_STRIKE, 7); }
void GameSoundSystem::playSfxSpare()              { playSfx(SFX_SPARE, 7); }
void GameSoundSystem::playSfxNeutralRoll()        { playSfx(SFX_NEUTRAL_ROLL, 4); }
xfm_voice_id GameSoundSystem::playSfxBallRolling() { return playSfx(SFX_BALL_ROLLING, 2); }
void GameSoundSystem::playSfxWin()                { playSfx(SFX_WIN, 7); }
void GameSoundSystem::playSfxLose()               { playSfx(SFX_LOSE, 7); }
void GameSoundSystem::playSfxBuy()                { playSfx(SFX_BUY, 6); }
void GameSoundSystem::playSfxTypewriter()         { playSfx(SFX_TYPEWRITER, 6); }

    // ------------------------------------------------------------------------
    // Volume
    // ------------------------------------------------------------------------

void GameSoundSystem::setMusicVolume(float v)
{
    musicVolume = v;
    if (musicModule) xfm_module_set_volume(musicModule, v);
}

void GameSoundSystem::setSfxVolume(float v)
{
    sfxVolume = v;
    if (sfxModule) xfm_module_set_volume(sfxModule, v);
}
    
    // ------------------------------------------------------------------------
    // Sound Settings UI
    // ------------------------------------------------------------------------
    
void GameSoundSystem::showSoundSettings()
{
    settings.activated = true;
}

void GameSoundSystem::hideSoundSettings()
{
    settings.activated = false;
}
    
// -----------------------------------------------------------------------------
// SoundSettings function implementations (must be after GameSoundSystem is defined)
// -----------------------------------------------------------------------------
