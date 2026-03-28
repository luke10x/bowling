#pragma once

#include <SDL.h>

#include "./../eggsfm/xfm_api.h"
#include "./../eggsfm/xfm_impl.cpp"
#include "./clayton/soundsettings.h"
#include "./sounds/songs_data.h"
#include <SDL.h>
#include <cstdio>
#include <cstring>

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

    SDL_AudioDeviceID audioDev = 0;

    float musicVolume = 1.0f;
    float sfxVolume   = 0.3f;
    int sampleRate = 44100;
    bool useWavPlayback = false;

    // Current song index (for switching between songs)
    int currentSongIndex = 1;

    // Sound settings UI
    SoundSettings settings;

    // ------------------------------------------------------------------------
    // Audio callback (mix both modules)
    // ------------------------------------------------------------------------

    static void audio_callback(void* userdata, Uint8* stream, int len)
    {
        GameSoundSystem* self = (GameSoundSystem*)userdata;

        // Safety check - if modules are null, just output silence
        if (!self->musicModule && !self->sfxModule) {
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
    }

    // ------------------------------------------------------------------------
    // Init
    // ------------------------------------------------------------------------

    bool initSoundSystem(const char* songPattern)
    {
        SDL_AudioSpec desired{};
        // Use the sampleRate setting (44100 for HiFi, 11025 for LoFi)
        desired.freq     = useWavPlayback ? 44100 : sampleRate;
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
        musicModule = xfm_module_create(obtained.freq, obtained.samples, XFM_CHIP_YM3438);
        sfxModule   = xfm_module_create(obtained.freq, obtained.samples, XFM_CHIP_YM3438);

        if (!musicModule || !sfxModule)
        {
            printf("xfm_module_create failed\n");
            return false;
        }

        // --------------------------------------------------------------------
        // Load patches (use XFM_CHIP_YM3438 to match module creation)
        // --------------------------------------------------------------------

        xfm_patch_set(musicModule, 0x00, &PATCH_00, sizeof(PATCH_00), XFM_CHIP_YM3438);
        xfm_patch_set(musicModule, 0x01, &PATCH_01, sizeof(PATCH_01), XFM_CHIP_YM3438);
        xfm_patch_set(musicModule, 0x02, &PATCH_02, sizeof(PATCH_02), XFM_CHIP_YM3438);  // Hi-hat channel

        // reuse for SFX
        xfm_patch_set(sfxModule, 0x00, &PATCH_00, sizeof(PATCH_00), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x01, &PATCH_01, sizeof(PATCH_01), XFM_CHIP_YM3438);
        xfm_patch_set(sfxModule, 0x02, &PATCH_02, sizeof(PATCH_02), XFM_CHIP_YM3438);

        // --------------------------------------------------------------------
        // Declare song
        // --------------------------------------------------------------------

        printf("Declaring song...\n");
        xfm_song_declare(musicModule, 1, songPattern, 60, 6);
        printf("Playing song...\n");
        xfm_song_play(musicModule, 1, true);
        printf("Music should be playing!\n");

        // --------------------------------------------------------------------
        // Declare SFX (patterns now use instrument 00)
        // --------------------------------------------------------------------

        xfm_sfx_declare(sfxModule, SFX_BALL_HIT_LANE,   SFX_PAT_BALL_HIT_LANE,   60, 3);
        xfm_sfx_declare(sfxModule, SFX_BALL_HIT_PINS,   SFX_PAT_BALL_HIT_PINS,   60, 3);
        xfm_sfx_declare(sfxModule, SFX_PIN_HIT_PIN,     SFX_PAT_PIN_HIT_PIN,     60, 3);
        xfm_sfx_declare(sfxModule, SFX_SCORE_DISPLAY,   SFX_PAT_SCORE_DISPLAY,   60, 3);
        xfm_sfx_declare(sfxModule, SFX_GUTTER,          SFX_PAT_GUTTER,          60, 3);
        xfm_sfx_declare(sfxModule, SFX_TIMEOUT,         SFX_PAT_TIMEOUT,         60, 3);

        // --------------------------------------------------------------------
        // Volume
        // --------------------------------------------------------------------

        xfm_module_set_volume(musicModule, musicVolume);
        xfm_module_set_volume(sfxModule, sfxVolume);

        // Initialize sound settings UI
        initSoundSettings(&settings, this);

        SDL_PauseAudioDevice(audioDev, 0);

        return true;
    }

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

    void shutdown()
    {
        // Pause audio first
        if (audioDev) {
            SDL_PauseAudioDevice(audioDev, 1);
        }

        if (audioDev)
        {
            SDL_CloseAudioDevice(audioDev);
            audioDev = 0;
        }

        if (musicModule)
        {
            xfm_module_destroy(musicModule);
            musicModule = nullptr;
        }

        if (sfxModule)
        {
            xfm_module_destroy(sfxModule);
            sfxModule = nullptr;
        }
    }

    // ------------------------------------------------------------------------
    // Restart sound system (for quality changes)
    // ------------------------------------------------------------------------

    bool restartSoundSystem()
    {
        // Pause audio device first to prevent callback during restart
        if (audioDev) {
            SDL_PauseAudioDevice(audioDev, 1);
        }

        // Wait for any pending callbacks to finish
        SDL_Delay(100);

        // Shutdown current audio
        shutdown();

        // Small delay after shutdown
        SDL_Delay(50);

        // Re-init with new settings
        const char* songPattern = nullptr;
        switch (currentSongIndex) {
            case 1: songPattern = SONG_01; break;
            case 2: songPattern = SONG_02; break;
            case 3: songPattern = SONG_03; break;
            case 4: songPattern = SONG_04; break;
            default: songPattern = SONG_01; break;
        }

        bool result = initSoundSystem(songPattern);

        // Audio device is unpaused by initSoundSystem
        return result;
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

        if (musicModule && songPattern) {
            // Declare and play new song (this replaces the current one)
            xfm_song_declare(musicModule, currentSongIndex, songPattern, 60, 6);
            xfm_song_play(musicModule, currentSongIndex, true);
            printf("Playing song %d\n", currentSongIndex);
        }
    }

    // ------------------------------------------------------------------------
    // SFX playback
    // ------------------------------------------------------------------------

    void playSfx(int id, int priority)
    {
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

    // Initialize from sound system
    self->musicVolume = soundSystem->musicVolume;
    self->sfxVolume = soundSystem->sfxVolume;
    self->quality = soundSystem->useWavPlayback ? SoundSettings::QUALITY_WAV :
                    (soundSystem->sampleRate == 44100 ? SoundSettings::QUALITY_HIFI : SoundSettings::QUALITY_LOFI);

    // Volume labels
    strcpy(self->musicVolLabels[0], "0%");
    strcpy(self->musicVolLabels[1], "25%");
    strcpy(self->musicVolLabels[2], "50%");
    strcpy(self->musicVolLabels[3], "75%");
    strcpy(self->musicVolLabels[4], "100%");

    memcpy(self->sfxVolLabels, self->musicVolLabels, sizeof(self->sfxVolLabels));

    // Quality labels
    strcpy(self->qualityLabels[0], "HiFi 44100");
    strcpy(self->qualityLabels[1], "LoFi 11025");
    strcpy(self->qualityLabels[2], "WAV (TODO)");

    // Initialize clicks
    const char* volIds[] = { "musicVol0", "musicVol1", "musicVol2", "musicVol3", "musicVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&self->musicVolClicks[i], volIds[i]);
    }

    const char* sfxIds[] = { "sfxVol0", "sfxVol1", "sfxVol2", "sfxVol3", "sfxVol4" };
    for (int i = 0; i < 5; i++) {
        initClaytonClick(&self->sfxVolClicks[i], sfxIds[i]);
    }

    const char* qualIds[] = { "qualHifi", "qualLofi", "qualWav" };
    for (int i = 0; i < 3; i++) {
        initClaytonClick(&self->qualityClicks[i], qualIds[i]);
    }

    initClaytonClick(&self->nextSongClick, "nextSongClick");
    initClaytonClick(&self->closeClick, "soundSettingsClose");
}

inline void applySoundSettings(SoundSettings* self)
{
    if (!self->soundSystem) return;

    self->soundSystem->musicVolume = self->musicVolume;
    self->soundSystem->sfxVolume = self->sfxVolume;

    // Apply volume to modules
    if (self->soundSystem->musicModule) {
        xfm_module_set_volume(self->soundSystem->musicModule, self->musicVolume);
    }
    if (self->soundSystem->sfxModule) {
        xfm_module_set_volume(self->soundSystem->sfxModule, self->sfxVolume);
    }

    // Quality change - just store the setting (no restart in Emscripten)
    // The Web Audio API handles sample rate internally
    switch (self->quality) {
        case SoundSettings::QUALITY_HIFI:
            self->soundSystem->sampleRate = 44100;
            self->soundSystem->useWavPlayback = false;
            printf("Quality set to HiFi (setting stored, audio continues at browser rate)\n");
            break;
        case SoundSettings::QUALITY_LOFI:
            self->soundSystem->sampleRate = 11025;
            self->soundSystem->useWavPlayback = false;
            printf("Quality set to LoFi (setting stored, audio continues at browser rate)\n");
            break;
        case SoundSettings::QUALITY_WAV:
            self->soundSystem->sampleRate = 44100;
            self->soundSystem->useWavPlayback = true;
            printf("Quality set to WAV (TODO - setting stored)\n");
            break;
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
        .textColor = {25, 25, 25, 255},
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
                    for (int i = 0; i < 5; i++) {
                        Clay_Color btnColor = (self->musicVolume == i * 0.25f) ?
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
                    for (int i = 0; i < 5; i++) {
                        Clay_Color btnColor = (self->sfxVolume == i * 0.25f) ?
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

            // Quality Section
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

// Song definitions are now in sounds/songs_data.h
// SONG1, SONG2, SONG3, SONG4 are defined as constexpr in songs_data.h
