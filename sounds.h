#pragma once
#include "../ingamefm/ingamefm.h"
#include <SDL.h>

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
    // Example SFX patterns (replace with your own later)
    // ------------------------------------------------------------------------

    static constexpr const char *PAT_BALL_HIT_LANE = "4\n"
                                                     "C-3107F\n"
                                                     ".......\n"
                                                     "OFF....\n"
                                                     ".......\n";

    static constexpr const char *PAT_BALL_HIT_PINS = "6\n"
                                                     "C-4107F\n"
                                                     "E-4107F\n"
                                                     "G-4107F\n"
                                                     "OFF....\n"
                                                     ".......\n"
                                                     ".......\n";

    static constexpr const char *PAT_PIN_HIT_PIN = "4\n"
                                                   "A-4107F\n"
                                                   "OFF....\n"
                                                   ".......\n"
                                                   ".......\n";

    static constexpr const char *PAT_SCORE_DISPLAY = "8\n"
                                                     "C-5127F\n"
                                                     "E-5127F\n"
                                                     "G-5127F\n"
                                                     "C-6127F\n"
                                                     "OFF....\n"
                                                     ".......\n"
                                                     ".......\n"
                                                     ".......\n";

    static constexpr const char *PAT_GUTTER = "8\n"
                                              "A-3107F\n"
                                              "F-3107F\n"
                                              "D-3107F\n"
                                              "OFF....\n"
                                              ".......\n"
                                              ".......\n"
                                              ".......\n"
                                              ".......\n";

    static constexpr const char *PAT_TIMEOUT = "6\n"
                                               "D-5117F\n"
                                               "OFF....\n"
                                               "A-4117F\n"
                                               "OFF....\n"
                                               ".......\n"
                                               ".......\n";

    // ------------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------------

    IngameFMPlayer player{};
    SDL_AudioDeviceID audioDev = 0;

    float musicVolume = 0.5f;
    float sfxVolume = 0.5f;

    // ------------------------------------------------------------------------
    // Init
    // ------------------------------------------------------------------------

    bool initSoundSystem(const char *songPattern)
    {
        SDL_AudioSpec desired{};
        desired.freq = IngameFMPlayer::SAMPLE_RATE;
        desired.format = AUDIO_S16SYS;
        desired.channels = 2;
        desired.samples = 128;
        desired.callback = IngameFMPlayer::s_audio_callback;
        desired.userdata = &player;

        SDL_AudioSpec obtained{};

        // audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        // if (audioDev == 0) {
        //     perror("audiodivice wrond ");
        //     return false;
        // }
        audioDev = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (audioDev == 0)
        {
            printf("Audio device error: %s\n", SDL_GetError());
            return false;
        }

        // music
        player.set_song(songPattern, 60, 20);

        player.add_patch(0x01, PATCH_KICK);
        player.add_patch(0x02, PATCH_SNARE);
        player.add_patch(0x03, PATCH_GUITAR);
        player.add_patch(0x04, PATCH_SLAP_BASS);
        player.add_patch(0x05, PATCH_FLUTE);
        player.add_patch(0x06, PATCH_SUPERSAW);
        player.add_patch(0x07, PATCH_ELECTRIC_BASS);

        player.add_patch(0x10, PATCH_SLAP_BASS);
        player.add_patch(0x11, PATCH_CLANG);
        player.add_patch(0x12, PATCH_GUITAR);
        player.add_patch(0x13, PATCH_SYNTH_BASS);
        player.add_patch(0x14, PATCH_HIHAT);
        player.add_patch(0x15, PATCH_ELECTRIC_BASS);

        // SFX voice pool
        player.sfx_set_voices(3);

        // define SFX

        player.sfx_define(SFX_BALL_HIT_LANE, PAT_BALL_HIT_LANE, 60, 3);
        player.sfx_define(SFX_BALL_HIT_PINS, PAT_BALL_HIT_PINS, 60, 3);
        player.sfx_define(SFX_PIN_HIT_PIN, PAT_PIN_HIT_PIN, 60, 3);
        player.sfx_define(SFX_SCORE_DISPLAY, PAT_SCORE_DISPLAY, 60, 3);
        player.sfx_define(SFX_GUTTER, PAT_GUTTER, 60, 3);
        player.sfx_define(SFX_TIMEOUT, PAT_TIMEOUT, 60, 3);

        // volumes start at 50%
        player.set_music_volume(musicVolume);
        player.set_sfx_volume(sfxVolume);

        player.start(audioDev, true);
        SDL_PauseAudioDevice(audioDev, 0);

        player.set_music_volume(0.1f);
        player.set_sfx_volume(0.5f);

        return true;
    }

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

    void shutdown()
    {
        if (audioDev)
        {
            player.stop(audioDev);
            SDL_CloseAudioDevice(audioDev);
            audioDev = 0;
        }
    }

    // ------------------------------------------------------------------------
    // Internal helper
    // ------------------------------------------------------------------------

    void playSfx(int id, int priority, int duration)
    {
        if (!audioDev) {
            perror("no audioded");

            return;
        }

        SDL_LockAudioDevice(audioDev);
        player.sfx_play(id, priority, duration);
        SDL_UnlockAudioDevice(audioDev);
    }

    // ------------------------------------------------------------------------
    // Game event hooks
    // ------------------------------------------------------------------------

    void playSfxBallHitLane()
    {
        printf("SFX trigger\n");

        playSfx(SFX_BALL_HIT_LANE, 3, 16);
    }

    void playSfxBallHitPins()
    {
        playSfx(SFX_BALL_HIT_PINS, 5, 10);
    }

    void playSfxPinHitsAnotherPin()
    {
        playSfx(SFX_PIN_HIT_PIN, 3, 6);
    }

    void playSfxFinalScoreDisplayed()
    {
        playSfx(SFX_SCORE_DISPLAY, 6, 12);
    }

    void playSfxBallInGutter()
    {
        playSfx(SFX_GUTTER, 5, 10);
    }

    void playSfxBallTimeout()
    {
        playSfx(SFX_TIMEOUT, 4, 8);
    }

    // ------------------------------------------------------------------------
    // Volume control
    // ------------------------------------------------------------------------

    void setMusicVolume(float v)
    {
        musicVolume = v;
        player.set_music_volume(v);
    }

    void setSfxVolume(float v)
    {
        sfxVolume = v;
        player.set_sfx_volume(v);
    }
};
