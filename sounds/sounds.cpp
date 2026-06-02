#include <SDL.h>

#include "./../../eggsfm/xfm_api.h"
#include "./../../eggsfm/xfm_wavplay.h"
#include "./../../eggsfm/xfm_export.h"

// #include "./assets/sound_out/all_wav_xxd.h"  // Disabled - WAVs now exported at runtime

#include <cstdio>
#include <cstring>
#include <atomic>
#include <algorithm>
#include <string>

// #include <clay.h>
// #include "../clayton/clayton_click.h"
// #include "../clayton/claytheme.h"

#include "./sounds.h"

// Forward declaration to break circular dependency with sounds.h
// struct GameSoundSystem;

static constexpr uint8_t MUSIC_BUILTIN_LEGACY_FIRST = 0x00;
static constexpr uint8_t MUSIC_BUILTIN_LEGACY_LAST = 0x13;
static constexpr uint8_t MUSIC_BUILTIN_HIGH_LAST = 0xFF;
static constexpr uint8_t MUSIC_BUILTIN_HIGH_FIRST = (uint8_t)(MUSIC_BUILTIN_HIGH_LAST - (MUSIC_BUILTIN_LEGACY_LAST - MUSIC_BUILTIN_LEGACY_FIRST));

static inline bool musicIsLegacyBuiltinInstrument(int inst)
{
    return inst >= MUSIC_BUILTIN_LEGACY_FIRST && inst <= MUSIC_BUILTIN_LEGACY_LAST;
}

static inline uint8_t musicHighIdFromLegacy(int legacyInst)
{
    return (uint8_t)(MUSIC_BUILTIN_HIGH_LAST - (uint8_t)legacyInst);
}

static inline std::string remapBuiltinMusicInstrumentIdsToHigh(const char *pattern)
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

    // Same scan strategy as TrackerSongIO_MarkReferencedInstruments:
    // skip leading whitespace, rowcount digits, and the rest of the first line.
    size_t i = 0;
    while (i < out.size() && (out[i] == ' ' || out[i] == '\t' || out[i] == '\n' || out[i] == '\r')) i++;
    while (i < out.size() && out[i] >= '0' && out[i] <= '9') i++;
    while (i < out.size() && out[i] != '\n') i++;
    if (i < out.size() && out[i] == '\n') i++;

    int columnPos = 0;
    for (; i < out.size(); i++)
    {
        char c = out[i];
        if (c == '\n')
        {
            columnPos = 0;
            continue;
        }
        if (c == '|')
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
                if (musicIsLegacyBuiltinInstrument(legacyInst))
                {
                    uint8_t newInst = musicHighIdFromLegacy(legacyInst);
                    out[i] = hexDigit(newInst >> 4);
                    out[i + 1] = hexDigit(newInst);
                }
            }
        }
        columnPos++;
    }
    return out;
}

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
    if (!remappedBuiltinSongPatternsReady)
    {
        remappedBuiltinSongPatterns[0] = remapBuiltinMusicInstrumentIdsToHigh(SONG_01);
        remappedBuiltinSongPatterns[1] = remapBuiltinMusicInstrumentIdsToHigh(SONG_02);
        remappedBuiltinSongPatterns[2] = remapBuiltinMusicInstrumentIdsToHigh(SONG_03);
        remappedBuiltinSongPatterns[3] = remapBuiltinMusicInstrumentIdsToHigh(SONG_04);
        remappedBuiltinSongPatternsReady = true;
    }
    switch (songIndex) {
        case 1: return remappedBuiltinSongPatterns[0].c_str();
        case 2: return remappedBuiltinSongPatterns[1].c_str();
        case 3: return remappedBuiltinSongPatterns[2].c_str();
        case 4: return remappedBuiltinSongPatterns[3].c_str();
        case TRACKER_USER_SONG_SLOT:
            return userSongVisible && userSongPattern[0] ? userSongPattern : remappedBuiltinSongPatterns[0].c_str();
        default: return remappedBuiltinSongPatterns[0].c_str();
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
            restartSongPattern = songPattern ? songPattern : getSongPattern(currentSongIndex);
            printf("[SoundRestart] Restart already in progress (state=%d), updated pending pattern\n", (int)restartState);
        } else {
            uint32_t elapsed = SDL_GetTicks64() - shutdownCompleteTime;
            restartSongPattern = songPattern ? songPattern : getSongPattern(currentSongIndex);
            restartState = RestartState::RESTART_WAIT_MORE;
            restartWaitFrames = 1;
            restartProgress = 0.6f;
            printf("[SoundRestart] Grace period not elapsed (%dms < %dms), queued restart\n", 
                    elapsed, GRACE_PERIOD_MS);
        }
        return;
    }
    restartSongPattern = songPattern ? songPattern : getSongPattern(currentSongIndex);
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

bool GameSoundSystem::reopenAudioDevice()
{
    if (audioDisabled)
        return true;
    if (audioDev)
    {
        audioShutdownInProgress.store(false);
        SDL_PauseAudioDevice(audioDev, 0);
        return true;
    }

    SDL_AudioSpec desired{};
    desired.freq = Sound_PreferredAudioSampleRate(*this);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = useWavPlayback ? WAV_PLAYBACK_BUFFER_SIZE : SYNTH_BUFFER_SIZE;
    desired.callback = my_audio_callback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (!audioDev)
    {
        printf("[SoundBrowser] Reopen audio device failed: %s\n", SDL_GetError());
        audioShutdownInProgress.store(true);
        return false;
    }

    obtainedSampleRate = obtained.freq > 0 ? obtained.freq : desired.freq;
    sampleRate = obtainedSampleRate;
    audioShutdownInProgress.store(false);
    SDL_PauseAudioDevice(audioDev, 0);
    printf("[SoundBrowser] Audio device reopened: %d Hz, %d samples\n", obtained.freq, obtained.samples);
    return true;
}

void GameSoundSystem::suspendForBrowser()
{
    if (browserAudioSuspended)
        return;
    // Remember whether music was playing, so a resume doesn't accidentally restart
    // user-stopped playback (common in the tracker UI).
    if (!useWavPlayback)
        musicWasActiveBeforeBrowserSuspend = musicModule && musicModule->active_song.active;
    else
        musicWasActiveBeforeBrowserSuspend = wavMusicModule && xfm_wav_song_is_playing(wavMusicModule);
    browserAudioSuspended = true;
    audioShutdownInProgress.store(true);
    if (audioDev)
    {
        SDL_CloseAudioDevice(audioDev);
        audioDev = 0;
        printf("[SoundBrowser] Audio device closed for browser suspend\n");
    }
}

void GameSoundSystem::resumeFromBrowser(const char* songPattern)
{
    if (audioDisabled)
    {
        browserAudioSuspended = false;
        return;
    }
    // Ignore spurious "resume" events (we listen to pointerdown/touchstart to satisfy autoplay),
    // but still allow this call to reopen the device if we currently have no audio device.
    if (!browserAudioSuspended && audioDev)
        return;
    if (restartState != RestartState::RESTART_IDLE && restartState != RestartState::RESTART_COMPLETE)
        return;

    audioShutdownInProgress.store(false);
    if (!musicModule && !wavMusicModule && !sfxModule && !wavSfxModule)
    {
        printf("[SoundBrowser] Modules missing on resume; reinitializing sound system\n");
        if (!initSoundSystem(songPattern ? songPattern : getSongPattern(currentSongIndex)))
            audioShutdownInProgress.store(true);
        browserAudioSuspended = false;
        return;
    }
    if (reopenAudioDevice())
    {
        browserAudioSuspended = false;
        trackerNeedsFullPatchSync = true;
        if (musicWasActiveBeforeBrowserSuspend)
            playCurrentMusic(false);
        return;
    }

    printf("[SoundBrowser] Reopen failed on resume; rebuilding sound system\n");
    shutdown();
    shutdownCompleteTime = 0;
    if (initSoundSystem(songPattern ? songPattern : getSongPattern(currentSongIndex)))
    {
        browserAudioSuspended = false;
        trackerNeedsFullPatchSync = true;
        if (!musicWasActiveBeforeBrowserSuspend)
            stopMusic();
    }
}

void GameSoundSystem::playCurrentMusic(bool restart)
{
    if (audioDisabled)
        return;
    if (!audioDev)
    {
        if (!reopenAudioDevice())
            return;
    }
    if (!useWavPlayback)
    {
        if (musicModule)
        {
            if (!audioDev)
                return;
            SDL_LockAudioDevice(audioDev);
            if (restart || musicModule->active_song.song_id != currentSongIndex)
                xfm_song_play(musicModule, currentSongIndex, true);
            else
                musicModule->active_song.active = true;
            if (musicLoopEndRow >= 0)
                xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
            SDL_UnlockAudioDevice(audioDev);
        }
    }
    else if (wavMusicModule)
    {
        xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
    }
}

void GameSoundSystem::startMusicAtRow(int row)
{
    if (audioDisabled)
        return;
    if (!audioDev && !reopenAudioDevice())
        return;
    if (!useWavPlayback)
    {
        if (!musicModule || !audioDev)
            return;
        SDL_LockAudioDevice(audioDev);
        if (musicModule->active_song.song_id != currentSongIndex)
            xfm_song_play(musicModule, currentSongIndex, true);
        if (musicLoopEndRow >= 0)
            xfm_song_set_loop_range(musicModule, musicLoopStartRow, musicLoopEndRow);
        musicModule->active_song.active = true;
        xfm_song_jump_to_row(musicModule, row);
        SDL_UnlockAudioDevice(audioDev);
    }
    else if (wavMusicModule)
    {
        xfm_wav_song_play(wavMusicModule, currentSongIndex, true);
    }
}

void GameSoundSystem::stopMusic()
{
    if (audioDisabled)
        return;
    if (!useWavPlayback)
    {
        if (musicModule)
        {
            if (!audioDev)
                return;
            SDL_LockAudioDevice(audioDev);
            musicModule->active_song.active = false;
            for (int ch = 0; ch < 6; ch++)
            {
                if (musicModule->chip)
                    musicModule->chip->key_off(ch);
                musicModule->channel_active[ch] = false;
            }
            SDL_UnlockAudioDevice(audioDev);
        }
    }
    else if (wavMusicModule)
    {
        xfm_wav_song_stop(wavMusicModule);
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
    sampleRate = obtainedSampleRate;

    // Any time we rebuild modules, the tracker must re-upload custom patches/macros.
    trackerNeedsFullPatchSync = true;

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

	        // Built-in music instruments live at the end of the 0..255 instrument bank.
	        // Legacy ids 0x00..0x13 map to 0xFF..0xEC (0xFF - legacy).
	        xfm_patch_set(musicModule, 0xFF, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xFE, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xFD, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
	
	        xfm_patch_set(musicModule, 0xFC, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xFB, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xFA, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF9, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF8, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF7, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
	
	        xfm_patch_set(musicModule, 0xF6, &PATCH_09_WAH, sizeof(PATCH_09_WAH), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF5, &PATCH_0A_GUITAR2, sizeof(PATCH_0A_GUITAR2), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF4, &PATCH_0B_BASS_KICK, sizeof(PATCH_0B_BASS_KICK), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF3, &PATCH_0C_TSH, sizeof(PATCH_0C_TSH), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF2, &PATCH_0D_TICK, sizeof(PATCH_0D_TICK), XFM_CHIP_YM3438);
	
	        xfm_patch_set(musicModule, 0xF1, &PATCH_0E_LEAD, sizeof(PATCH_0E_LEAD), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xF0, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xEF, &PATCH_10_HARDBASS, sizeof(PATCH_10_HARDBASS), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xEE, &PATCH_11_LOWBASS, sizeof(PATCH_11_LOWBASS), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xED, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438);
	        xfm_patch_set(musicModule, 0xEC, &PATCH_13_ROLL, sizeof(PATCH_13_ROLL), XFM_CHIP_YM3438);

        // reuse for SFX
        xfm_patch_set(sfxModule, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);  // Hi-hat channel
        xfm_patch_set(sfxModule, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x0F, &PATCH_0F_KICK, sizeof(PATCH_0F_KICK), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x12, &PATCH_12_AXE, sizeof(PATCH_12_AXE), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x13, &PATCH_13_ROLL, sizeof(PATCH_13_ROLL), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x14, &PATCH_14_GLASS_CRACK, sizeof(PATCH_14_GLASS_CRACK), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x15, &PATCH_15_GLASS_SCRAPE, sizeof(PATCH_15_GLASS_SCRAPE), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x16, &PATCH_16_GLASS_SHARD, sizeof(PATCH_16_GLASS_SHARD), XFM_CHIP_YM3438);
        xfm_module_set_lfo(sfxModule, true, 5);
    }

    // --------------------------------------------------------------------
    // Declare song
    // --------------------------------------------------------------------

        // Built-in songs (1..4) always use the remapped patterns so instrument ids
        // match the built-in patch bank at 0xEC..0xFF.
        const bool isBuiltinSong = currentSongIndex >= 1 && currentSongIndex <= TRACKER_BUILTIN_SONG_COUNT;
        const char *effectiveSongPattern =
            isBuiltinSong ? getSongPattern(currentSongIndex) : (songPattern ? songPattern : getSongPattern(currentSongIndex));

	    if (!this->useWavPlayback) {
	        printf("Declaring song...\n");
	        int songTicksPerStep = currentSongIndex == 2 ? 8 : 6;
	        xfm_song_declare(musicModule, currentSongIndex, effectiveSongPattern, 60, songTicksPerStep);
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
        xfm_sfx_declare(sfxModule, SFX_GLASS_CRACK,     SFX_PAT_GLASS_CRACK,     60, 1);
        xfm_sfx_declare(sfxModule, SFX_GLASS_SCRAPE,    SFX_PAT_GLASS_SCRAPE,    60, 1);
        xfm_sfx_declare(sfxModule, SFX_GLASS_SHARDS,    SFX_PAT_GLASS_SHARDS,    60, 1);
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
    if (!audioDev && !reopenAudioDevice()) return FM_VOICE_INVALID;

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

xfm_voice_id GameSoundSystem::previewTrackerNote(
    int note,
    int octave,
    int instrument,
    int volume,
    const xfm_patch_opn *patchOverride,
    const XfmMacro *macros,
    const bool *macroEnabled,
    const bool *macroValid,
    bool held
)
{
    if (audioDisabled || useWavPlayback) return FM_VOICE_INVALID;
    if (!sfxModule) return FM_VOICE_INVALID;
    if (!audioDev && !reopenAudioDevice()) return FM_VOICE_INVALID;

    static const char *names[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
    int safeNote = std::max(0, std::min(11, note));
    int safeOctave = std::max(1, std::min(7, octave));
    int safeInstrument = std::max(0, std::min(255, instrument));
    int safeVolume = std::max(0, std::min(127, volume));
    const int previewInstrument = 0xEB;

    const xfm_patch_opn *sourcePatch = patchOverride;
    if (!sourcePatch && musicModule && musicModule->patch_present[safeInstrument])
        sourcePatch = &musicModule->patches[safeInstrument];
    if (!sourcePatch && sfxModule->patch_present[safeInstrument])
        sourcePatch = &sfxModule->patches[safeInstrument];
    if (!sourcePatch) return FM_VOICE_INVALID;

    xfm_patch_opn previewPatch = *sourcePatch;
    int tlAdd = ((0x7F - safeVolume) * 127) / 0x7F;
    for (int op = 0; op < 4; op++)
        previewPatch.op[op].TL = (uint8_t)std::min(127, (int)previewPatch.op[op].TL + tlAdd);

    SDL_LockAudioDevice(audioDev);
    xfm_patch_set(sfxModule, previewInstrument, &previewPatch, sizeof(previewPatch), XFM_CHIP_YM3438);
    xfm_patch_macro_clear(sfxModule, previewInstrument, XFM_MACRO_NONE);
    if (macros && macroEnabled && macroValid)
    {
        int macroId = 0;
        for (int target = XFM_MACRO_TL1; target < XFM_MACRO_TARGET_COUNT && macroId < XFM_MAX_MACROS; target++)
        {
            if (!macroEnabled[target] || !macroValid[target])
                continue;
            XfmMacro macro = macros[target];
            macro.target = (uint8_t)target;
            if (macro.length == 0)
                macro.length = 1;
            if (macro.length > XFM_MAX_MACRO_VALUES)
                macro.length = XFM_MAX_MACRO_VALUES;
            if (macro.has_loop && macro.loop_start >= macro.length)
                macro.loop_start = macro.length - 1;
            if (macro.release_start != 0xFF && macro.release_start >= macro.length)
                macro.release_start = macro.length - 1;
            if (xfm_macro_set(sfxModule, macroId, &macro) >= 0)
            {
                xfm_patch_macro_set(sfxModule, previewInstrument, (uint8_t)target, macroId);
                macroId++;
            }
        }
    }

    if (trackerPreviewVoice != FM_VOICE_INVALID)
    {
        xfm_sfx_stop(sfxModule, trackerPreviewVoice);
        trackerPreviewVoice = FM_VOICE_INVALID;
    }

    xfm_voice_id voice = FM_VOICE_INVALID;
    if (held)
    {
        // Long-running SFX that sustains until the caller explicitly stops it.
        std::string pattern = "4096\n";
        char firstRow[16];
        std::snprintf(firstRow, sizeof(firstRow), "%s%d%02X7F\n", names[safeNote], safeOctave, previewInstrument);
        pattern += firstRow;
        for (int row = 1; row < 4096; row++)
            pattern += ".......\n";

        xfm_sfx_declare(sfxModule, SFX_TRACKER_PREVIEW, pattern.c_str(), 60, 1);
        voice = xfm_sfx_play(sfxModule, SFX_TRACKER_PREVIEW, /*priority=*/0);
    }
    else
    {
        // Short preview: note, then a REL within ~1 row time.
        char pattern[128];
        std::snprintf(
            pattern,
            sizeof(pattern),
            "4\n%s%d%02X7F\n.......\nREL....\n.......\n",
            names[safeNote],
            safeOctave,
            previewInstrument
        );
        xfm_sfx_declare(sfxModule, SFX_TRACKER_PREVIEW, pattern, 60, 1);
        voice = xfm_sfx_play(sfxModule, SFX_TRACKER_PREVIEW, /*priority=*/0);
    }
    trackerPreviewVoice = voice;
    SDL_UnlockAudioDevice(audioDev);
    return voice;
}

void GameSoundSystem::releaseTrackerPreviewNote()
{
    if (audioDisabled || useWavPlayback || !sfxModule || !audioDev)
        return;
    SDL_LockAudioDevice(audioDev);
    if (trackerPreviewVoice != FM_VOICE_INVALID)
    {
        xfm_sfx_stop(sfxModule, trackerPreviewVoice);
        trackerPreviewVoice = FM_VOICE_INVALID;
    }
    SDL_UnlockAudioDevice(audioDev);
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
void GameSoundSystem::playSfxGlassBreak()
{
    if (useWavPlayback) return;
    playSfx(SFX_GLASS_CRACK, 8);
    playSfx(SFX_GLASS_SCRAPE, 7);
    playSfx(SFX_GLASS_SHARDS, 7);
}

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
