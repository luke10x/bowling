#pragma once

#include "../clayton/clayton.h"
#include "./sounds.h"

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

inline void buildSoundSettingsWindowClay(Clayton *clayton, SoundSettings *self)
{
    if (!self->activated)
    {
        return;
    }

    // Font configs - use theme
    Clay_TextElementConfig labelFontCfg = CLAY_THEME_TEXT_LABEL;
    Clay_TextElementConfig buttonFontCfg = CLAY_THEME_TEXT_BUTTON;
    Clay_TextElementConfig titleFontCfg = CLAY_THEME_TEXT_TITLE;

    // Main container exists for pointer-hit testing in processSoundSettingsEvent().
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
    )
    {
        // Settings panel window
        CLAY(
            CLAY_ID("SoundSettingsWindow"),
            {
                .layout =
                    {
                        .sizing = {CLAY_SIZING_PERCENT(0.8f), CLAY_SIZING_FIT()},
                        .padding = {20, 20, 20, 20},
                        .childGap = 15,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                .backgroundColor = CLAY_COLOR_PANEL_BG,
                .cornerRadius = {CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL, CLAY_RADIUS_XL},
                CLAY_THEME_WINDOW_BORDER
            }
        )
        {
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
            )
            {
                CLAY_TEXT(CLAY_STRING("Sound Settings"), CLAY_TEXT_CONFIG(titleFontCfg));

                /* -------- DIVIDER -------- */
                CLAY(
                    CLAY_ID("SoundSettingsTitleDivider"),
                    {
                        .layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(1)}},
                    }
                ){};

                // Close button (right side)
                CLAY(clayton->closeClick.clayId, CLAY_THEME_BTN_DANGER)
                {
                    CLAY_TEXT(CLAY_STRING("X"), CLAY_TEXT_CONFIG(CLAY_THEME_TEXT_BUTTON));
                }
            }

            // Quality Section OR Restart Progress (mutually exclusive)
            if (self->soundSystem && self->soundSystem->restartProgress > 0.0f &&
                self->soundSystem->restartProgress < 1.0f)
            {
                // Show progress indicator instead of quality buttons during restart
                CLAY(
                    CLAY_ID("RestartProgressSection"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .padding = {10, 10, 10, 10},
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        .backgroundColor = {80, 60, 40, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                )
                {
                    Clay_TextElementConfig progressFontCfg = {
                        .textColor = {255, 255, 100, 255},
                        .fontId = CLAY_FONT_NOTO,
                        .fontSize = (uint16_t)18,
                    };
                    CLAY_TEXT(
                        CLAY_STRING("Changing quality..."), CLAY_TEXT_CONFIG(progressFontCfg)
                    );

                    // Progress bar background
                    CLAY(
                        CLAY_ID("ProgressBarBg"),
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(20)},
                                },
                            .backgroundColor = {40, 40, 40, 255},
                            .cornerRadius = {5, 5, 5, 5},
                        }
                    )
                    {
                        // Progress bar fill
                        float progress = self->soundSystem->restartProgress;
                        Clay_Color progressColor;
                        if (progress < 0.5f)
                        {
                            progressColor = {200, 200, 50, 255}; // Yellow
                        }
                        else if (progress < 0.8f)
                        {
                            progressColor = {200, 150, 50, 255}; // Orange
                        }
                        else
                        {
                            progressColor = {50, 200, 50, 255}; // Green
                        }

                        CLAY(
                            CLAY_ID("ProgressBarFill"),
                            {
                                .layout =
                                    {
                                        .sizing =
                                            {CLAY_SIZING_PERCENT(progress), CLAY_SIZING_GROW()},
                                    },
                                .backgroundColor = progressColor,
                                .cornerRadius = {5, 5, 5, 5},
                            }
                        ){};
                    }

                    // Progress percentage text
                    char progressText[20];
                    int progressLen = snprintf(
                        progressText,
                        sizeof(progressText),
                        "%d%%",
                        (int)(self->soundSystem->restartProgress * 100)
                    );
                    Clay_String progressStr = {
                        .isStaticallyAllocated = false,
                        .length = progressLen,
                        .chars = progressText,
                    };
                    CLAY_TEXT(progressStr, CLAY_TEXT_CONFIG(progressFontCfg));
                }
            }
            else
            {
                // Show quality buttons when not restarting
                CLAY(
                    CLAY_ID("QualitySection"),
                    {
                        .layout =
                            {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                .padding = {10, 10, 10, 10},
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        .backgroundColor = {60, 60, 80, 255},
                        .cornerRadius = {10, 10, 10, 10},
                    }
                )
                {
                    CLAY_TEXT(CLAY_STRING("Audio Mode"), CLAY_TEXT_CONFIG(labelFontCfg));

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
                    )
                    {
                        for (int i = 0; i < 2; i++)
                        {
                            Clay_Color btnColor = (self->quality == i)
                                ? Clay_Color{100, 200, 100, 255}
                                : Clay_Color{80, 80, 120, 255};

                            CLAY(
                                clayton->qualityClicks[i].clayId,
                                {
                                    .layout =
                                        {
                                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                            .childAlignment =
                                                {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                        },
                                    .backgroundColor = btnColor,
                                    .cornerRadius = {8, 8, 8, 8},
                                }
                            )
                            {
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
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            )
            {
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
                )
                {
                    // Find which button should be highlighted (closest to current volume)
                    int selectedButton = -1;
                    float minDiff = 1.0f;
                    for (int i = 0; i < 5; i++)
                    {
                        float targetVol = i * 0.25f;
                        float diff = fabsf(self->musicVolume - targetVol);
                        if (diff < minDiff)
                        {
                            minDiff = diff;
                            selectedButton = i;
                        }
                    }

                    for (int i = 0; i < 5; i++)
                    {
                        Clay_Color btnColor = (i == selectedButton) ? Clay_Color{100, 200, 100, 255}
                                                                    : Clay_Color{80, 80, 120, 255};

                        CLAY(
                            clayton->musicVolClicks[i].clayId,
                            {
                                .layout =
                                    {
                                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
                                        .childAlignment =
                                            {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                    },
                                .backgroundColor = btnColor,
                                .cornerRadius = {8, 8, 8, 8},
                            }
                        )
                        {
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
            // CLAY(
            //     CLAY_ID("SfxVolSection"),
            //     {
            //         .layout = {
            //             .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
            //             .padding = {10, 10, 10, 10},
            //             .childGap = 10,
            //             .layoutDirection = CLAY_TOP_TO_BOTTOM,
            //         },
            //         .backgroundColor = {60, 60, 80, 255},
            //         .cornerRadius = {10, 10, 10, 10},
            //     }
            // ) {
            //     CLAY_TEXT(CLAY_STRING("SFX Volume"), CLAY_TEXT_CONFIG(labelFontCfg));

            //     // Volume buttons row
            //     CLAY(
            //         CLAY_ID("SfxVolRow"),
            //         {
            //             .layout = {
            //                 .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
            //                 .childGap = 8,
            //                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
            //             },
            //         }
            //     ) {
            //         // Find which button should be highlighted (closest to current volume)
            //         int selectedButton = -1;
            //         float minDiff = 1.0f;
            //         for (int i = 0; i < 5; i++) {
            //             float targetVol = i * 0.25f;
            //             float diff = fabsf(self->sfxVolume - targetVol);
            //             if (diff < minDiff) {
            //                 minDiff = diff;
            //                 selectedButton = i;
            //             }
            //         }

            //         for (int i = 0; i < 5; i++) {
            //             Clay_Color btnColor = (i == selectedButton) ?
            //                 Clay_Color{100, 200, 100, 255} : Clay_Color{80, 80, 120, 255};

            //             CLAY(
            //                 self->sfxVolClicks[i].clayId,
            //                 {
            //                     .layout = {
            //                         .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(50)},
            //                         .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            //                     },
            //                     .backgroundColor = btnColor,
            //                     .cornerRadius = {8, 8, 8, 8},
            //                     .border = {
            //                         .color = {150, 150, 200, 255},
            //                         .width = CLAY_BORDER_ALL(2),
            //                     },
            //                 }
            //             ) {
            //                 Clay_String label = {
            //                     .isStaticallyAllocated = false,
            //                     .length = (int)strlen(self->sfxVolLabels[i]),
            //                     .chars = self->sfxVolLabels[i],
            //                 };
            //                 CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonFontCfg));
            //             }
            //         }
            //     }
            // }

            CLAY(
                CLAY_ID("SongSection"),
                {
                    .layout =
                        {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .padding = {10, 10, 10, 10},
                            .childGap = 10,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    .backgroundColor = {60, 60, 80, 255},
                    .cornerRadius = {10, 10, 10, 10},
                }
            )
            {
                CLAY_TEXT(CLAY_STRING("Song"), CLAY_TEXT_CONFIG(labelFontCfg));

                // Action buttons row
                CLAY(
                    CLAY_ID("ActionRow"),
                    {
                        .layout = {
                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                            .childGap = 10,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                    }
                )
                {
                    // Previous Song button (left side)
                    CLAY(
                        clayton->prevSongClick.clayId,
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(60)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {50, 100, 200, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("◀"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }

                    // Song name display (center)
                    CLAY(
                        CLAY_ID("SongNameDisplay"),
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(60)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {30, 30, 50, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    )
                    {
                        Clay_String songName = {
                            .isStaticallyAllocated = false,
                            .length = (int)strlen(self->currentSongName),
                            .chars = self->currentSongName,
                        };
                        Clay_TextElementConfig songNameCfg = {
                            .textColor = {200, 200, 255, 255},
                            .fontId = CLAY_FONT_NOTO,
                            .fontSize = CLAY_FONT_SIZE_SM,
                        };
                        CLAY_TEXT(songName, CLAY_TEXT_CONFIG(songNameCfg));
                    }

                    // Next Song button (right side)
                    CLAY(
                        clayton->nextSongClick.clayId,
                        {
                            .layout =
                                {
                                    .sizing = {CLAY_SIZING_FIXED(60), CLAY_SIZING_FIXED(60)},
                                    .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                },
                            .backgroundColor = {50, 100, 200, 255},
                            .cornerRadius = {10, 10, 10, 10},
                        }
                    )
                    {
                        CLAY_TEXT(CLAY_STRING("▶"), CLAY_TEXT_CONFIG(buttonFontCfg));
                    }
                }
            }
        }
    }
}

// Legacy wrapper: preserves the old "dims background" behavior for call sites that expect it.
inline void buildSoundSettingsClay(Clayton *clayton, SoundSettings *self)
{
    if (!self || !self->activated)
    {
        return;
    }
    CLAY(CLAY_ID("SoundSettingsContainerOverlay"), CLAY_THEME_OVERLAY)
    {
        buildSoundSettingsWindowClay(clayton, self);
    }
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
