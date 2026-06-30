# Adding a New SFX to the Game

This document describes the current file-based SFX workflow. SFX now live as
built-in tracker DSL header files under [`/Users/lape/workspace/bowling/sounds/builtin_sfx`](/Users/lape/workspace/bowling/sounds/builtin_sfx), and runtime remaps each file's local instrument ids into one shared global SFX instrument bank so overlapping SFX do not stomp each other.

## Overview

The audio system has two modes:
- **Synth mode**: Real-time YM3438 OPN chip synthesis
- **Cached (WAV) mode**: Pre-generated WAV blobs played from memory

Both modes must be updated when adding a new SFX.

## Step-by-Step Procedure

### 1. Create a Built-In SFX File

Add a new file under [`/Users/lape/workspace/bowling/sounds/builtin_sfx`](/Users/lape/workspace/bowling/sounds/builtin_sfx). Each file is a valid tracker DSL header and should contain:

```cpp
#pragma once
#include <xfm_song_dsl.h>

XFM_SONG_BEGIN(R"xfmname(Coin Pickup)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
4
E-5007F
G-5007F
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Rubber Bass
...
ENDINST
)xfminstruments")
XFM_SONG_END()
```

Rules:
- SFX should use only the first tracker channel.
- Local instrument ids may start at `00` inside the file.
- Runtime will remap those local ids into a shared global SFX bank automatically.
- Keep the row count tracker-valid. Very long loop SFX should stay within the tracker parser limit.
- Mirror the shared SFX chip `XFM_TICK_RATE`, `XFM_SPEED`, `XFM_LFO_ENABLED`, and `XFM_LFO_FREQUENCY`
  in each file for tracker/editing consistency, but remember runtime treats those as global chip settings
  and will override per-file differences.

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
    SFX_COIN_PICKUP, // <-- new
    SFX_COUNT
};
```

Keep `SFX_COUNT` as the final enum entry. It is used by the cached WAV path, so most buffer sizes and loops do not need hard-coded edits.

### 3. Check Buffer Sizes (`sounds/sounds.h`)

Runtime SFX buffers should be sized with `SFX_COUNT`:

```cpp
void* runtimeSfxBuffers[SFX_COUNT] = {};
int runtimeSfxSizes[SFX_COUNT] = {};
```

The `setRuntimeWavBuffers` signature should also use `SFX_COUNT`:

```cpp
void setRuntimeWavBuffers(void* songs[4], int songSizes[4], void* sfxs[SFX_COUNT], int sfxSizes[SFX_COUNT]);
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

### 5. Register the File (`sounds/builtin_sfx_registry.h`)

Add the new built-in file to [`/Users/lape/workspace/bowling/sounds/builtin_sfx_registry.h`](/Users/lape/workspace/bowling/sounds/builtin_sfx_registry.h):

```cpp
namespace BuiltinSfxFileCoinPickup
{
#include "builtin_sfx/coin_pickup.h"
}

{ SFX_COIN_PICKUP, "coin_pickup", "sounds/builtin_sfx/coin_pickup.h", ... }
```

### 6. Check WAV Loading Loop (`sounds/sounds.cpp`)

WAV loading should loop over `SFX_COUNT`:

```cpp
for (int i = 0; i < SFX_COUNT; i++) {
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
void* sfxBuffers[GameSoundSystem::SFX_COUNT];
int sfxBufferSizes[GameSoundSystem::SFX_COUNT];
```

### 8. Adaptive Audio and Export Paths

The synth path, adaptive cached-WAV path, and standalone WAV exporter now read
from the built-in SFX registry and the shared prepared SFX bank. In most cases,
adding the registry entry is enough; avoid reintroducing hardcoded `SFX_PAT_*`
tables.

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

- **Synth mode**: SFX files are parsed into a shared global instrument bank, then declared with remapped pattern text.
- **WAV mode**: Adaptive audio export uses the same registry and remapped pattern text.
- **One source of truth**: the built-in SFX file plus its registry entry.
- **Buffer sizes** should use `SFX_COUNT`; avoid adding new hard-coded SFX totals.
- **SFX priority** (second arg to `playSfx`) determines which SFX plays when multiple compete for the same channel.

## Long or Cancellable SFX

For a long SFX that must stop on a gameplay event, expose a play function that returns the voice handle:

```cpp
xfm_voice_id GameSoundSystem::playSfxBallRolling() {
    return playSfx(SFX_BALL_ROLLING, 2);
}
```

Store that handle in game state and stop it explicitly:

```cpp
xfm_voice_id rollingVoice = sound.playSfxBallRolling();
sound.stopSfx(rollingVoice);
rollingVoice = FM_VOICE_INVALID;
```

This works in both synth and cached WAV modes because `stopSfx()` routes to the active backend. For cached WAV mode, make the source pattern long enough for the expected maximum duration and stop it when gameplay says the sound is over.
