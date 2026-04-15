# Adding a New SFX to the Game

This document describes the procedure for adding a new sound effect (SFX) to the game. The coin pickup SFX is used as an example.

## Overview

The audio system has two modes:
- **Synth mode**: Real-time YM3438 OPN chip synthesis
- **Cached (WAV) mode**: Pre-generated WAV blobs played from memory

Both modes must be updated when adding a new SFX.

## Step-by-Step Procedure

### 1. Define the SFX Pattern (`sounds/songs_data.h`)

Add a new `SFX_PAT_*` constant with the instrument pattern:

```cpp
// Coin pickup - bright ascending blip
constexpr const char* SFX_PAT_COIN_PICKUP = "4\n"
                                             "E-5007F\n"
                                             "G-5007F\n"
                                             "OFF....\n"
                                             ".......\n";
```

The pattern format:
- First line: number of rows (steps)
- Each row: note + octave + instrument + flags (e.g., `E-5007F`)
- `OFF....` = note off
- `.......` = rest

### 2. Add the SFX Enum (`sounds/sounds.h`)

Add a new entry to the `SfxId` enum:

```cpp
enum SfxId
{
    SFX_BALL_HIT_LANE = 0,
    SFX_BALL_HIT_PINS,
    SFX_PIN_HIT_PIN,
    SFX_SCORE_DISPLAY,
    SFX_GUTTER,
    SFX_TIMEOUT,
    SFX_COIN_PICKUP  // <-- new
};
```

### 3. Update Buffer Sizes (`sounds/sounds.h`)

Increase the runtime SFX buffer arrays from 6 to 7:

```cpp
void* runtimeSfxBuffers[7] = {nullptr, ...};
int runtimeSfxSizes[7] = {0, 0, 0, 0, 0, 0, 0};
```

Update the `setRuntimeWavBuffers` signature:

```cpp
void setRuntimeWavBuffers(void* songs[4], int songSizes[4], void* sfxs[7], int sfxSizes[7]);
```

### 4. Add the Play Function (`sounds/sounds.h` + `sounds/sounds.cpp`)

Declaration in `sounds.h`:
```cpp
void playSfxCoinPickup();
```

Implementation in `sounds.cpp`:
```cpp
void GameSoundSystem::playSfxCoinPickup() { playSfx(SFX_COIN_PICKUP, 4); }
```

### 5. Declare the SFX in Init (`sounds/sounds.cpp`)

Add the `xfm_sfx_declare` call in `initSoundSystem()`:

```cpp
xfm_sfx_declare(sfxModule, SFX_COIN_PICKUP, SFX_PAT_COIN_PICKUP, 60, 3);
```

### 6. Update WAV Loading Loop (`sounds/sounds.cpp`)

Change the loop from 6 to 7:

```cpp
for (int i = 0; i < 7; i++) {
    if (runtimeSfxBuffers[i] && runtimeSfxSizes[i] > 0) {
        // load SFX i...
    }
}
```

### 7. Update Adaptive Audio Export (`sounds/adaptive_audio.h`)

Add new export step enum entries:

```cpp
EXPORT_STEP_SFX_7_BEGIN,
EXPORT_STEP_SFX_7_STEP,
EXPORT_STEP_SFX_7_FINALIZE,
```

Update buffer sizes in `AdaptiveAudioSystem`:
```cpp
void* sfxBuffers[7];
int sfxBufferSizes[7];
```

### 8. Update Adaptive Audio Export Logic (`sounds/adaptive_audio.cpp`)

Update the SFX pattern arrays:
```cpp
const char* sfxPatternsInit[] = {
    ..., SFX_PAT_COIN_PICKUP
};
// Change loop from 6 to 7
for (int i = 0; i < 7; i++) { ... }

const char* sfxPatternsArr[] = {
    ..., SFX_PAT_COIN_PICKUP
};
int sfxIdsArr[] = { 0, 1, 2, 3, 4, 5, 6 };
```

Update status text from "6" to "7":
```cpp
snprintf(self->exportStatus, ..., "Caching SFX %d/7...", sfxIdx + 1);
```

Add state machine entries:
```cpp
case EXPORT_STEP_SFX_7_BEGIN: SFX_BEGIN(6, 6)
case EXPORT_STEP_SFX_7_STEP: SFX_STEP(6)
case EXPORT_STEP_SFX_7_FINALIZE: SFX_FINALIZE(6)
```

### 9. Trigger the SFX in Game Code (`game.cpp`)

Add a counter to `UserContext`:
```cpp
int coinsCollectedThisLane = 0;
```

Detect newly collected coins and play SFX:
```cpp
// Count collected coins before update
int collectedBefore = 0;
for (int i = 0; i < usr->coinLane.getActiveCount(); i++) {
    if (usr->coinLane.getCoins()[i].collected) collectedBefore++;
}

usr->coinLane.updateStars(ballModel[3], usr->globalTime, deltaTime);

// Count newly collected coins and play SFX for each
int collectedAfter = 0;
for (int i = 0; i < usr->coinLane.getActiveCount(); i++) {
    if (usr->coinLane.getCoins()[i].collected) collectedAfter++;
}
int newCollected = collectedAfter - collectedBefore;
for (int i = 0; i < newCollected; i++) {
    usr->sound.playSfxCoinPickup();
}
```

Reset counter when coins respawn:
```cpp
if (usr->coinLane.autoRespawnIfNeeded(getRandomCoinPattern(), 7, deltaTime)) {
    usr->coinsCollectedThisLane = 0;
}
```

## Key Points

- **Synth mode**: SFX is declared via `xfm_sfx_declare` and played via `playSfx()`
- **WAV mode**: SFX is pre-generated during adaptive audio export and loaded via `xfm_wav_load_memory`
- **Both modes must be updated** - the export state machine in `adaptive_audio.cpp` must include the new SFX
- **Buffer sizes** must be increased everywhere (sounds.h, adaptive_audio.h, sounds.cpp, adaptive_audio.cpp)
- **SFX priority** (second arg to `playSfx`) determines which SFX plays when multiple compete for the same channel
