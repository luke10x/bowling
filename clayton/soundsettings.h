#pragma once

#include <SDL.h>
#include <clay.h>

#include "./clayton_click.h"
#include "../../eggsfm/xfm_api.h"

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
        QUALITY_HIFI = 0,    // 44100 Hz realtime
        QUALITY_LOFI = 1,    // 11025 Hz realtime
        QUALITY_WAV = 2      // WAV fallback
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
