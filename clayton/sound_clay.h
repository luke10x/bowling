#include "./clayton.h"

inline void initSoundSettings(Clayton *clayton, SoundSettings *soundSettingsState, GameSoundSystem *soundSystem)
{
    soundSettingsState->soundSystem = soundSystem;
    // self->activated = false;

    // Initialize from sound system - read ACTUAL current values
    soundSettingsState->musicVolume = soundSystem->musicVolume;
    soundSettingsState->sfxVolume = soundSystem->sfxVolume;

    // Determine current quality mode from sound system state
    if (soundSystem->useWavPlayback)
    {
        soundSettingsState->quality = SoundSettings::QUALITY_WAV;
        // } else if (soundSystem->sampleRate == 11025) {
        //     self->quality = SoundSettings::QUALITY_LOFI;
    }
    else
    {
        soundSettingsState->quality = SoundSettings::QUALITY_HIFI;
    }

    printf(
        "[SoundSettings] Initialized: musicVol=%.2f, sfxVol=%.2f, quality=%d\n",
        soundSettingsState->musicVolume,
        soundSettingsState->sfxVolume,
        (int)soundSettingsState->quality
    );

    // Volume labels
    strcpy(soundSettingsState->musicVolLabels[0], "0%");
    strcpy(soundSettingsState->musicVolLabels[1], "25%");
    strcpy(soundSettingsState->musicVolLabels[2], "50%");
    strcpy(soundSettingsState->musicVolLabels[3], "75%");
    strcpy(soundSettingsState->musicVolLabels[4], "100%");

    memcpy(soundSettingsState->sfxVolLabels, soundSettingsState->musicVolLabels, sizeof(soundSettingsState->sfxVolLabels));

    // Quality labels
    strcpy(soundSettingsState->qualityLabels[0], "Cached");
    // strcpy(self->qualityLabels[1], "LoFi 11025");
    strcpy(soundSettingsState->qualityLabels[1], "Synth");

    // Initialize clicks
    const char *volIds[] = {"musicVol0", "musicVol1", "musicVol2", "musicVol3", "musicVol4"};
    for (int i = 0; i < 5; i++)
    {
        initClaytonClick(&clayton->musicVolClicks[i], volIds[i]);
    }

    const char *sfxIds[] = {"sfxVol0", "sfxVol1", "sfxVol2", "sfxVol3", "sfxVol4"};
    for (int i = 0; i < 5; i++)
    {
        initClaytonClick(&clayton->sfxVolClicks[i], sfxIds[i]);
    }

    const char *qualIds[] = {
        "qualWav",
        // "qualLofi",
        "qualHifi",
    };
    for (int i = 0; i < 2; i++)
    {
        initClaytonClick(&clayton->qualityClicks[i], qualIds[i]);
    }

    initClaytonClick(&clayton->nextSongClick, "nextSongClick");
    initClaytonClick(&clayton->prevSongClick, "prevSongClick");
    initClaytonClick(&clayton->closeClick, "soundSettingsClose");
    initClaytonClick(&clayton->hiScoreCloseClick, "hiScoreCloseClose");

    // Song names - fun random names for each track
    strcpy(soundSettingsState->songNames[1], "1. Bowling Strike");
    strcpy(soundSettingsState->songNames[2], "2. Gutter Groove");
    strcpy(soundSettingsState->songNames[3], "3. Pin Crusher");
    strcpy(soundSettingsState->songNames[4], "4. Alley Cat");

    // Set initial song name
    strcpy(soundSettingsState->currentSongName, soundSettingsState->songNames[soundSettingsState->soundSystem->currentSongIndex]);

    // Initialize WAV export flag
    soundSettingsState->needsWavExport = false;
    soundSettingsState->wavExportInProgress = false;
    soundSettingsState->wavExportStatus[0] = '\0';
}


inline void applySoundSettings(SoundSettings *soundSettingsClay)
{
    if (!soundSettingsClay->soundSystem)
        return;

    // Apply volume to modules immediately (no restart needed)
    // Volume changes do NOT affect quality setting
    if (soundSettingsClay->soundSystem->musicModule)
    {
        xfm_module_set_volume(soundSettingsClay->soundSystem->musicModule, soundSettingsClay->musicVolume);
    }
    if (soundSettingsClay->soundSystem->sfxModule)
    {
        xfm_module_set_volume(soundSettingsClay->soundSystem->sfxModule, soundSettingsClay->sfxVolume);
    }
    // WAV volume control
    if (soundSettingsClay->soundSystem->wavMusicModule)
    {
        printf("[SoundVolume] WAV music volume: %.2f\n", soundSettingsClay->musicVolume);
        xfm_wav_module_set_volume(soundSettingsClay->soundSystem->wavMusicModule, soundSettingsClay->musicVolume);
    }
    if (soundSettingsClay->soundSystem->wavSfxModule)
    {
        printf("[SoundVolume] WAV SFX volume: %.2f\n", soundSettingsClay->sfxVolume);
        xfm_wav_module_set_volume(soundSettingsClay->soundSystem->wavSfxModule, soundSettingsClay->sfxVolume);
    }

    // Check current mode BEFORE applying new setting
    bool wasWav = soundSettingsClay->soundSystem->useWavPlayback;
    int wasSampleRate = soundSettingsClay->soundSystem->sampleRate;

    // Apply new quality setting
    bool wantsWav = false;
    int wantsSampleRate = 44100;

    switch (soundSettingsClay->quality)
    {
    case SoundSettings::QUALITY_HIFI:
        wantsWav = false;
        wantsSampleRate = 44100;
        soundSettingsClay->soundSystem->sampleRate = 44100;
        printf("[SoundSettings] Quality requested: HiFi 44100 (synth)\n");
        break;
    // case SoundSettings::QUALITY_LOFI:
    //     wantsWav = false;
    //     wantsSampleRate = 11025;
    //     self->soundSystem->sampleRate = 11025;
    //     printf("[SoundSettings] Quality requested: LoFi 11025 (synth)\n");
    //     break;
    case SoundSettings::QUALITY_WAV:
        wantsWav = true;
        wantsSampleRate = 11025; // WAV always uses 44100
        wantsSampleRate = 44100; // WAV always uses 44100
        soundSettingsClay->soundSystem->sampleRate = 11025;
        soundSettingsClay->soundSystem->sampleRate = 44100;
        printf("[SoundSettings] Quality requested: WAV (pre-rendered)\n");
        break;
    }

    // Check if mode actually changed (WAV flag OR sample rate)
    bool modeChanged = (wantsWav != wasWav) || (wantsSampleRate != wasSampleRate);

    if (modeChanged)
    {
        printf(
            "[SoundSettings] Mode CHANGED (WAV=%d→%d, Rate=%d→%d) - scheduling restart...\n",
            wasWav,
            wantsWav,
            wasSampleRate,
            wantsSampleRate
        );

        // Apply new mode immediately (will take effect after restart)
        soundSettingsClay->soundSystem->useWavPlayback = wantsWav;

        // If switching to WAV but buffers aren't loaded, trigger export first
        if (wantsWav && !soundSettingsClay->soundSystem->hasRuntimeWavBuffers)
        {
            printf("[SoundSettings] WAV selected but buffers not loaded - triggering export...\n");
            soundSettingsClay->needsWavExport = true;
            // Don't restart yet - export will trigger restart when done
            return;
        }

        // Get current song pattern for restart
        const char *songPattern = SONG_01;
        switch (soundSettingsClay->soundSystem->currentSongIndex)
        {
        case 1:
            songPattern = SONG_01;
            break;
        case 2:
            songPattern = SONG_02;
            break;
        case 3:
            songPattern = SONG_03;
            break;
        case 4:
            songPattern = SONG_04;
            break;
        }

        soundSettingsClay->soundSystem->startRestart(songPattern);
    }
    else
    {
        printf("[SoundSettings] Mode unchanged (no restart needed)\n");
    }
}

inline bool processSoundSettingsEvent(Clayton *clayton, SoundSettings *soundSettingsClay, SDL_Event event)
{
    if (!soundSettingsClay->activated)
    {
        return false;
    }

    bool mouseDown = event.type == SDL_MOUSEBUTTONDOWN;
    bool mouseUp = event.type == SDL_MOUSEBUTTONUP;

    if (!mouseDown && !mouseUp)
    {
        return false;
    }

    bool handled = false;

    // Music volume buttons
    for (int i = 0; i < 5; i++)
    {
        if (isClaytonClicked(&clayton->musicVolClicks[i], event))
        {
            soundSettingsClay->musicVolume = i * 0.25f;
            applySoundSettings(soundSettingsClay);
            handled = true;
        }
    }

    // // SFX volume buttons
    // for (int i = 0; i < 5; i++) {
    //     if (isClaytonClicked(&self->sfxVolClicks[i], event)) {
    //         self->sfxVolume = i * 0.25f;
    //         applySoundSettings(self);
    //         handled = true;
    //     }
    // }

    // Quality buttons
    for (int i = 0; i < 3; i++)
    {
        if (isClaytonClicked(&clayton->qualityClicks[i], event))
        {
            soundSettingsClay->quality = (SoundSettings::Quality)i;
            applySoundSettings(soundSettingsClay);
            handled = true;
        }
    }

    // Next song button
    if (isClaytonClicked(&clayton->nextSongClick, event))
    {
        if (soundSettingsClay->soundSystem)
        {
            soundSettingsClay->soundSystem->nextSong();
        }
        handled = true;
    }

    // Previous song button
    if (isClaytonClicked(&clayton->prevSongClick, event))
    {
        if (soundSettingsClay->soundSystem)
        {
            soundSettingsClay->soundSystem->previousSong();
        }
        handled = true;
    }

    // Close button
    if (isClaytonClicked(&clayton->closeClick, event))
    {
        soundSettingsClay->activated = false;
        return true;
    }

    // If pointer is over the panel, consume the event (even if not on a button)
    // This prevents click-through to the game
    if (Clay_PointerOver(CLAY_ID("SoundSettingsContainer")))
    {
        return true;
    }

    return handled;
}