#include "./clayton.h"

inline void initSoundSettings(Clayton *clayton, SoundSettings *self, GameSoundSystem *soundSystem)
{
    self->soundSystem = soundSystem;
    // self->activated = false;

    // Initialize from sound system - read ACTUAL current values
    self->musicVolume = soundSystem->musicVolume;
    self->sfxVolume = soundSystem->sfxVolume;

    // Determine current quality mode from sound system state
    if (soundSystem->useWavPlayback)
    {
        self->quality = SoundSettings::QUALITY_WAV;
        // } else if (soundSystem->sampleRate == 11025) {
        //     self->quality = SoundSettings::QUALITY_LOFI;
    }
    else
    {
        self->quality = SoundSettings::QUALITY_HIFI;
    }

    printf(
        "[SoundSettings] Initialized: musicVol=%.2f, sfxVol=%.2f, quality=%d\n",
        self->musicVolume,
        self->sfxVolume,
        (int)self->quality
    );

    // Volume labels
    strcpy(self->musicVolLabels[0], "0%");
    strcpy(self->musicVolLabels[1], "25%");
    strcpy(self->musicVolLabels[2], "50%");
    strcpy(self->musicVolLabels[3], "75%");
    strcpy(self->musicVolLabels[4], "100%");

    memcpy(self->sfxVolLabels, self->musicVolLabels, sizeof(self->sfxVolLabels));

    // Quality labels
    strcpy(self->qualityLabels[0], "Cached");
    // strcpy(self->qualityLabels[1], "LoFi 11025");
    strcpy(self->qualityLabels[1], "Synth");

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
    strcpy(self->songNames[1], "1. Bowling Strike");
    strcpy(self->songNames[2], "2. Gutter Groove");
    strcpy(self->songNames[3], "3. Pin Crusher");
    strcpy(self->songNames[4], "4. Alley Cat");

    // Set initial song name
    strcpy(self->currentSongName, self->songNames[self->soundSystem->currentSongIndex]);

    // Initialize WAV export flag
    self->needsWavExport = false;
    self->wavExportInProgress = false;
    self->wavExportStatus[0] = '\0';
}


inline void applySoundSettings(SoundSettings *self)
{
    if (!self->soundSystem)
        return;

    // Apply volume to modules immediately (no restart needed)
    // Volume changes do NOT affect quality setting
    if (self->soundSystem->musicModule)
    {
        xfm_module_set_volume(self->soundSystem->musicModule, self->musicVolume);
    }
    if (self->soundSystem->sfxModule)
    {
        xfm_module_set_volume(self->soundSystem->sfxModule, self->sfxVolume);
    }
    // WAV volume control
    if (self->soundSystem->wavMusicModule)
    {
        printf("[SoundVolume] WAV music volume: %.2f\n", self->musicVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavMusicModule, self->musicVolume);
    }
    if (self->soundSystem->wavSfxModule)
    {
        printf("[SoundVolume] WAV SFX volume: %.2f\n", self->sfxVolume);
        xfm_wav_module_set_volume(self->soundSystem->wavSfxModule, self->sfxVolume);
    }

    // Check current mode BEFORE applying new setting
    bool wasWav = self->soundSystem->useWavPlayback;
    int wasSampleRate = self->soundSystem->sampleRate;

    // Apply new quality setting
    bool wantsWav = false;
    int wantsSampleRate = 44100;

    switch (self->quality)
    {
    case SoundSettings::QUALITY_HIFI:
        wantsWav = false;
        wantsSampleRate = 44100;
        self->soundSystem->sampleRate = 44100;
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
        self->soundSystem->sampleRate = 11025;
        self->soundSystem->sampleRate = 44100;
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
        self->soundSystem->useWavPlayback = wantsWav;

        // If switching to WAV but buffers aren't loaded, trigger export first
        if (wantsWav && !self->soundSystem->hasRuntimeWavBuffers)
        {
            printf("[SoundSettings] WAV selected but buffers not loaded - triggering export...\n");
            self->needsWavExport = true;
            // Don't restart yet - export will trigger restart when done
            return;
        }

        // Get current song pattern for restart
        const char *songPattern = SONG_01;
        switch (self->soundSystem->currentSongIndex)
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

        self->soundSystem->startRestart(songPattern);
    }
    else
    {
        printf("[SoundSettings] Mode unchanged (no restart needed)\n");
    }
}

inline bool processSoundSettingsEvent(Clayton *clayton, SoundSettings *self, SDL_Event event)
{
    if (!self->activated)
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
            self->musicVolume = i * 0.25f;
            applySoundSettings(self);
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
            self->quality = (SoundSettings::Quality)i;
            applySoundSettings(self);
            handled = true;
        }
    }

    // Next song button
    if (isClaytonClicked(&clayton->nextSongClick, event))
    {
        if (self->soundSystem)
        {
            self->soundSystem->nextSong();
        }
        handled = true;
    }

    // Previous song button
    if (isClaytonClicked(&clayton->prevSongClick, event))
    {
        if (self->soundSystem)
        {
            self->soundSystem->previousSong();
        }
        handled = true;
    }

    // Close button
    if (isClaytonClicked(&clayton->closeClick, event))
    {
        self->activated = false;
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