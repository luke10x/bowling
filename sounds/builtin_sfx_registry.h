#pragma once

#include <cstddef>

namespace BuiltinSfxFileBallHitLane
{
#include "builtin_sfx/ball_hit_lane.h"
}

namespace BuiltinSfxFileBallHitPins
{
#include "builtin_sfx/ball_hit_pins.h"
}

namespace BuiltinSfxFilePinHitPin
{
#include "builtin_sfx/pin_hit_pin.h"
}

namespace BuiltinSfxFileScoreDisplay
{
#include "builtin_sfx/score_display.h"
}

namespace BuiltinSfxFileGutter
{
#include "builtin_sfx/gutter.h"
}

namespace BuiltinSfxFileTimeout
{
#include "builtin_sfx/timeout.h"
}

namespace BuiltinSfxFileCoinPickup
{
#include "builtin_sfx/coin_pickup.h"
}

namespace BuiltinSfxFileStrike
{
#include "builtin_sfx/strike.h"
}

namespace BuiltinSfxFileSpare
{
#include "builtin_sfx/spare.h"
}

namespace BuiltinSfxFileNeutralRoll
{
#include "builtin_sfx/neutral_roll.h"
}

namespace BuiltinSfxFileBallRolling
{
#include "builtin_sfx/ball_rolling.h"
}

namespace BuiltinSfxFileNosLoop
{
#include "builtin_sfx/nos_loop.h"
}

namespace BuiltinSfxFileWin
{
#include "builtin_sfx/win.h"
}

namespace BuiltinSfxFileLose
{
#include "builtin_sfx/lose.h"
}

namespace BuiltinSfxFileBuy
{
#include "builtin_sfx/buy.h"
}

namespace BuiltinSfxFileTypewriter
{
#include "builtin_sfx/typewriter.h"
}

namespace BuiltinSfxFileGlassCrack
{
#include "builtin_sfx/glass_crack.h"
}

namespace BuiltinSfxFileGlassScrape
{
#include "builtin_sfx/glass_scrape.h"
}

namespace BuiltinSfxFileGlassShards
{
#include "builtin_sfx/glass_shards.h"
}

struct BuiltinSfxDefinition
{
    int sfxId;
    const char *assetStem;
    const char *sourcePath;
    const char *displayName;
    const char *pattern;
    const char *instruments;
    int tickRate;
    int speed;
    bool lfoEnabled;
    int lfoFrequency;
};

static constexpr BuiltinSfxDefinition BUILTIN_SFX_REGISTRY[] = {
    {0,  "ball_hit_lane",  "sounds/builtin_sfx/ball_hit_lane.h",  BuiltinSfxFileBallHitLane::XFM_TRACKER_SONG_NAME,  BuiltinSfxFileBallHitLane::XFM_TRACKER_SONG_PATTERN,  BuiltinSfxFileBallHitLane::XFM_TRACKER_CUSTOM_INSTRUMENTS,  BuiltinSfxFileBallHitLane::XFM_TRACKER_TICK_RATE,  BuiltinSfxFileBallHitLane::XFM_TRACKER_SPEED,  BuiltinSfxFileBallHitLane::XFM_TRACKER_LFO_ENABLED != 0,  BuiltinSfxFileBallHitLane::XFM_TRACKER_LFO_FREQUENCY},
    {1,  "ball_hit_pins",  "sounds/builtin_sfx/ball_hit_pins.h",  BuiltinSfxFileBallHitPins::XFM_TRACKER_SONG_NAME,  BuiltinSfxFileBallHitPins::XFM_TRACKER_SONG_PATTERN,  BuiltinSfxFileBallHitPins::XFM_TRACKER_CUSTOM_INSTRUMENTS,  BuiltinSfxFileBallHitPins::XFM_TRACKER_TICK_RATE,  BuiltinSfxFileBallHitPins::XFM_TRACKER_SPEED,  BuiltinSfxFileBallHitPins::XFM_TRACKER_LFO_ENABLED != 0,  BuiltinSfxFileBallHitPins::XFM_TRACKER_LFO_FREQUENCY},
    {2,  "pin_hit_pin",    "sounds/builtin_sfx/pin_hit_pin.h",    BuiltinSfxFilePinHitPin::XFM_TRACKER_SONG_NAME,    BuiltinSfxFilePinHitPin::XFM_TRACKER_SONG_PATTERN,    BuiltinSfxFilePinHitPin::XFM_TRACKER_CUSTOM_INSTRUMENTS,    BuiltinSfxFilePinHitPin::XFM_TRACKER_TICK_RATE,    BuiltinSfxFilePinHitPin::XFM_TRACKER_SPEED,    BuiltinSfxFilePinHitPin::XFM_TRACKER_LFO_ENABLED != 0,    BuiltinSfxFilePinHitPin::XFM_TRACKER_LFO_FREQUENCY},
    {3,  "score_display",  "sounds/builtin_sfx/score_display.h",  BuiltinSfxFileScoreDisplay::XFM_TRACKER_SONG_NAME,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_SONG_PATTERN,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_CUSTOM_INSTRUMENTS,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_TICK_RATE,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_SPEED,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_LFO_ENABLED != 0,  BuiltinSfxFileScoreDisplay::XFM_TRACKER_LFO_FREQUENCY},
    {4,  "gutter",         "sounds/builtin_sfx/gutter.h",         BuiltinSfxFileGutter::XFM_TRACKER_SONG_NAME,         BuiltinSfxFileGutter::XFM_TRACKER_SONG_PATTERN,         BuiltinSfxFileGutter::XFM_TRACKER_CUSTOM_INSTRUMENTS,         BuiltinSfxFileGutter::XFM_TRACKER_TICK_RATE,         BuiltinSfxFileGutter::XFM_TRACKER_SPEED,         BuiltinSfxFileGutter::XFM_TRACKER_LFO_ENABLED != 0,         BuiltinSfxFileGutter::XFM_TRACKER_LFO_FREQUENCY},
    {5,  "timeout",        "sounds/builtin_sfx/timeout.h",        BuiltinSfxFileTimeout::XFM_TRACKER_SONG_NAME,        BuiltinSfxFileTimeout::XFM_TRACKER_SONG_PATTERN,        BuiltinSfxFileTimeout::XFM_TRACKER_CUSTOM_INSTRUMENTS,        BuiltinSfxFileTimeout::XFM_TRACKER_TICK_RATE,        BuiltinSfxFileTimeout::XFM_TRACKER_SPEED,        BuiltinSfxFileTimeout::XFM_TRACKER_LFO_ENABLED != 0,        BuiltinSfxFileTimeout::XFM_TRACKER_LFO_FREQUENCY},
    {6,  "coin_pickup",    "sounds/builtin_sfx/coin_pickup.h",    BuiltinSfxFileCoinPickup::XFM_TRACKER_SONG_NAME,    BuiltinSfxFileCoinPickup::XFM_TRACKER_SONG_PATTERN,    BuiltinSfxFileCoinPickup::XFM_TRACKER_CUSTOM_INSTRUMENTS,    BuiltinSfxFileCoinPickup::XFM_TRACKER_TICK_RATE,    BuiltinSfxFileCoinPickup::XFM_TRACKER_SPEED,    BuiltinSfxFileCoinPickup::XFM_TRACKER_LFO_ENABLED != 0,    BuiltinSfxFileCoinPickup::XFM_TRACKER_LFO_FREQUENCY},
    {7,  "strike",         "sounds/builtin_sfx/strike.h",         BuiltinSfxFileStrike::XFM_TRACKER_SONG_NAME,         BuiltinSfxFileStrike::XFM_TRACKER_SONG_PATTERN,         BuiltinSfxFileStrike::XFM_TRACKER_CUSTOM_INSTRUMENTS,         BuiltinSfxFileStrike::XFM_TRACKER_TICK_RATE,         BuiltinSfxFileStrike::XFM_TRACKER_SPEED,         BuiltinSfxFileStrike::XFM_TRACKER_LFO_ENABLED != 0,         BuiltinSfxFileStrike::XFM_TRACKER_LFO_FREQUENCY},
    {8,  "spare",          "sounds/builtin_sfx/spare.h",          BuiltinSfxFileSpare::XFM_TRACKER_SONG_NAME,          BuiltinSfxFileSpare::XFM_TRACKER_SONG_PATTERN,          BuiltinSfxFileSpare::XFM_TRACKER_CUSTOM_INSTRUMENTS,          BuiltinSfxFileSpare::XFM_TRACKER_TICK_RATE,          BuiltinSfxFileSpare::XFM_TRACKER_SPEED,          BuiltinSfxFileSpare::XFM_TRACKER_LFO_ENABLED != 0,          BuiltinSfxFileSpare::XFM_TRACKER_LFO_FREQUENCY},
    {9,  "neutral_roll",   "sounds/builtin_sfx/neutral_roll.h",   BuiltinSfxFileNeutralRoll::XFM_TRACKER_SONG_NAME,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_SONG_PATTERN,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_CUSTOM_INSTRUMENTS,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_TICK_RATE,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_SPEED,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_LFO_ENABLED != 0,   BuiltinSfxFileNeutralRoll::XFM_TRACKER_LFO_FREQUENCY},
    {10, "ball_rolling",   "sounds/builtin_sfx/ball_rolling.h",   BuiltinSfxFileBallRolling::XFM_TRACKER_SONG_NAME,   BuiltinSfxFileBallRolling::XFM_TRACKER_SONG_PATTERN,   BuiltinSfxFileBallRolling::XFM_TRACKER_CUSTOM_INSTRUMENTS,   BuiltinSfxFileBallRolling::XFM_TRACKER_TICK_RATE,   BuiltinSfxFileBallRolling::XFM_TRACKER_SPEED,   BuiltinSfxFileBallRolling::XFM_TRACKER_LFO_ENABLED != 0,   BuiltinSfxFileBallRolling::XFM_TRACKER_LFO_FREQUENCY},
    {11, "nos_loop",       "sounds/builtin_sfx/nos_loop.h",       BuiltinSfxFileNosLoop::XFM_TRACKER_SONG_NAME,       BuiltinSfxFileNosLoop::XFM_TRACKER_SONG_PATTERN,       BuiltinSfxFileNosLoop::XFM_TRACKER_CUSTOM_INSTRUMENTS,       BuiltinSfxFileNosLoop::XFM_TRACKER_TICK_RATE,       BuiltinSfxFileNosLoop::XFM_TRACKER_SPEED,       BuiltinSfxFileNosLoop::XFM_TRACKER_LFO_ENABLED != 0,       BuiltinSfxFileNosLoop::XFM_TRACKER_LFO_FREQUENCY},
    {12, "win",            "sounds/builtin_sfx/win.h",            BuiltinSfxFileWin::XFM_TRACKER_SONG_NAME,            BuiltinSfxFileWin::XFM_TRACKER_SONG_PATTERN,            BuiltinSfxFileWin::XFM_TRACKER_CUSTOM_INSTRUMENTS,            BuiltinSfxFileWin::XFM_TRACKER_TICK_RATE,            BuiltinSfxFileWin::XFM_TRACKER_SPEED,            BuiltinSfxFileWin::XFM_TRACKER_LFO_ENABLED != 0,            BuiltinSfxFileWin::XFM_TRACKER_LFO_FREQUENCY},
    {13, "lose",           "sounds/builtin_sfx/lose.h",           BuiltinSfxFileLose::XFM_TRACKER_SONG_NAME,           BuiltinSfxFileLose::XFM_TRACKER_SONG_PATTERN,           BuiltinSfxFileLose::XFM_TRACKER_CUSTOM_INSTRUMENTS,           BuiltinSfxFileLose::XFM_TRACKER_TICK_RATE,           BuiltinSfxFileLose::XFM_TRACKER_SPEED,           BuiltinSfxFileLose::XFM_TRACKER_LFO_ENABLED != 0,           BuiltinSfxFileLose::XFM_TRACKER_LFO_FREQUENCY},
    {14, "buy",            "sounds/builtin_sfx/buy.h",            BuiltinSfxFileBuy::XFM_TRACKER_SONG_NAME,            BuiltinSfxFileBuy::XFM_TRACKER_SONG_PATTERN,            BuiltinSfxFileBuy::XFM_TRACKER_CUSTOM_INSTRUMENTS,            BuiltinSfxFileBuy::XFM_TRACKER_TICK_RATE,            BuiltinSfxFileBuy::XFM_TRACKER_SPEED,            BuiltinSfxFileBuy::XFM_TRACKER_LFO_ENABLED != 0,            BuiltinSfxFileBuy::XFM_TRACKER_LFO_FREQUENCY},
    {15, "typewriter",     "sounds/builtin_sfx/typewriter.h",     BuiltinSfxFileTypewriter::XFM_TRACKER_SONG_NAME,     BuiltinSfxFileTypewriter::XFM_TRACKER_SONG_PATTERN,     BuiltinSfxFileTypewriter::XFM_TRACKER_CUSTOM_INSTRUMENTS,     BuiltinSfxFileTypewriter::XFM_TRACKER_TICK_RATE,     BuiltinSfxFileTypewriter::XFM_TRACKER_SPEED,     BuiltinSfxFileTypewriter::XFM_TRACKER_LFO_ENABLED != 0,     BuiltinSfxFileTypewriter::XFM_TRACKER_LFO_FREQUENCY},
    {16, "glass_crack",    "sounds/builtin_sfx/glass_crack.h",    BuiltinSfxFileGlassCrack::XFM_TRACKER_SONG_NAME,    BuiltinSfxFileGlassCrack::XFM_TRACKER_SONG_PATTERN,    BuiltinSfxFileGlassCrack::XFM_TRACKER_CUSTOM_INSTRUMENTS,    BuiltinSfxFileGlassCrack::XFM_TRACKER_TICK_RATE,    BuiltinSfxFileGlassCrack::XFM_TRACKER_SPEED,    BuiltinSfxFileGlassCrack::XFM_TRACKER_LFO_ENABLED != 0,    BuiltinSfxFileGlassCrack::XFM_TRACKER_LFO_FREQUENCY},
    {17, "glass_scrape",   "sounds/builtin_sfx/glass_scrape.h",   BuiltinSfxFileGlassScrape::XFM_TRACKER_SONG_NAME,   BuiltinSfxFileGlassScrape::XFM_TRACKER_SONG_PATTERN,   BuiltinSfxFileGlassScrape::XFM_TRACKER_CUSTOM_INSTRUMENTS,   BuiltinSfxFileGlassScrape::XFM_TRACKER_TICK_RATE,   BuiltinSfxFileGlassScrape::XFM_TRACKER_SPEED,   BuiltinSfxFileGlassScrape::XFM_TRACKER_LFO_ENABLED != 0,   BuiltinSfxFileGlassScrape::XFM_TRACKER_LFO_FREQUENCY},
    {18, "glass_shards",   "sounds/builtin_sfx/glass_shards.h",   BuiltinSfxFileGlassShards::XFM_TRACKER_SONG_NAME,   BuiltinSfxFileGlassShards::XFM_TRACKER_SONG_PATTERN,   BuiltinSfxFileGlassShards::XFM_TRACKER_CUSTOM_INSTRUMENTS,   BuiltinSfxFileGlassShards::XFM_TRACKER_TICK_RATE,   BuiltinSfxFileGlassShards::XFM_TRACKER_SPEED,   BuiltinSfxFileGlassShards::XFM_TRACKER_LFO_ENABLED != 0,   BuiltinSfxFileGlassShards::XFM_TRACKER_LFO_FREQUENCY},
};

static constexpr int BUILTIN_SFX_REGISTRY_COUNT =
    (int)(sizeof(BUILTIN_SFX_REGISTRY) / sizeof(BUILTIN_SFX_REGISTRY[0]));

inline const BuiltinSfxDefinition *BuiltinSfx_ByIndex(int index)
{
    if (index < 0 || index >= BUILTIN_SFX_REGISTRY_COUNT)
        return nullptr;
    return &BUILTIN_SFX_REGISTRY[index];
}

inline const BuiltinSfxDefinition *BuiltinSfx_ById(int sfxId)
{
    for (int i = 0; i < BUILTIN_SFX_REGISTRY_COUNT; ++i)
        if (BUILTIN_SFX_REGISTRY[i].sfxId == sfxId)
            return &BUILTIN_SFX_REGISTRY[i];
    return nullptr;
}
