#pragma once

#include <cstddef>

namespace BuiltinSongFile01
{
#include "builtin_songs/song_01.h"
}

namespace BuiltinSongFile02
{
#include "builtin_songs/song_02.h"
}

namespace BuiltinSongFile03
{
#include "builtin_songs/song_03.h"
}

namespace BuiltinSongFile04
{
#include "builtin_songs/song_04.h"
}

namespace BuiltinSongFile05
{
#include "builtin_songs/song_05.h"
}

// Legacy aliases kept so the rest of the codebase can migrate gradually while
// built-in songs now come from the user-facing DSL source files directly.
static constexpr const char *SONG_01_NAME = BuiltinSongFile01::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_01_TICK_RATE = BuiltinSongFile01::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_01_SPEED = BuiltinSongFile01::XFM_TRACKER_SPEED;
static constexpr int SONG_01_ROWS_PER_BEAT = BuiltinSongFile01::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_01_SCALE_ROOT = BuiltinSongFile01::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_01_SCALE_MODE = BuiltinSongFile01::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_01_LFO_ENABLED = BuiltinSongFile01::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_01_LFO_FREQUENCY = BuiltinSongFile01::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_01_INSTRUMENTS = BuiltinSongFile01::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_01 = BuiltinSongFile01::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_02_NAME = BuiltinSongFile02::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_02_TICK_RATE = BuiltinSongFile02::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_02_SPEED = BuiltinSongFile02::XFM_TRACKER_SPEED;
static constexpr int SONG_02_ROWS_PER_BEAT = BuiltinSongFile02::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_02_SCALE_ROOT = BuiltinSongFile02::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_02_SCALE_MODE = BuiltinSongFile02::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_02_LFO_ENABLED = BuiltinSongFile02::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_02_LFO_FREQUENCY = BuiltinSongFile02::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_02_INSTRUMENTS = BuiltinSongFile02::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_02 = BuiltinSongFile02::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_03_NAME = BuiltinSongFile03::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_03_TICK_RATE = BuiltinSongFile03::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_03_SPEED = BuiltinSongFile03::XFM_TRACKER_SPEED;
static constexpr int SONG_03_ROWS_PER_BEAT = BuiltinSongFile03::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_03_SCALE_ROOT = BuiltinSongFile03::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_03_SCALE_MODE = BuiltinSongFile03::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_03_LFO_ENABLED = BuiltinSongFile03::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_03_LFO_FREQUENCY = BuiltinSongFile03::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_03_INSTRUMENTS = BuiltinSongFile03::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_03 = BuiltinSongFile03::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_04_NAME = BuiltinSongFile04::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_04_TICK_RATE = BuiltinSongFile04::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_04_SPEED = BuiltinSongFile04::XFM_TRACKER_SPEED;
static constexpr int SONG_04_ROWS_PER_BEAT = BuiltinSongFile04::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_04_SCALE_ROOT = BuiltinSongFile04::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_04_SCALE_MODE = BuiltinSongFile04::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_04_LFO_ENABLED = BuiltinSongFile04::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_04_LFO_FREQUENCY = BuiltinSongFile04::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_04_INSTRUMENTS = BuiltinSongFile04::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_04 = BuiltinSongFile04::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_05_NAME = BuiltinSongFile05::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_05_TICK_RATE = BuiltinSongFile05::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_05_SPEED = BuiltinSongFile05::XFM_TRACKER_SPEED;
static constexpr int SONG_05_ROWS_PER_BEAT = BuiltinSongFile05::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_05_SCALE_ROOT = BuiltinSongFile05::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_05_SCALE_MODE = BuiltinSongFile05::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_05_LFO_ENABLED = BuiltinSongFile05::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_05_LFO_FREQUENCY = BuiltinSongFile05::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_05_INSTRUMENTS = BuiltinSongFile05::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_05 = BuiltinSongFile05::XFM_TRACKER_SONG_PATTERN;

struct BuiltinSongDefinition
{
    const char *codeStem;
    const char *sourcePath;
    const char *displayName;
    const char *pattern;
    const char *instruments;
    int tickRate;
    int speed;
    int rowsPerBeat;
    int scaleRoot;
    int scaleMode;
    bool lfoEnabled;
    int lfoFrequency;
};

#define BUILTIN_SONG_ENTRY(stem, path, ns) \
    { \
        stem, \
        path, \
        ns::XFM_TRACKER_SONG_NAME, \
        ns::XFM_TRACKER_SONG_PATTERN, \
        ns::XFM_TRACKER_CUSTOM_INSTRUMENTS, \
        ns::XFM_TRACKER_TICK_RATE, \
        ns::XFM_TRACKER_SPEED, \
        ns::XFM_TRACKER_ROWS_PER_BEAT, \
        ns::XFM_TRACKER_SCALE_ROOT, \
        ns::XFM_TRACKER_SCALE_MODE, \
        ns::XFM_TRACKER_LFO_ENABLED != 0, \
        ns::XFM_TRACKER_LFO_FREQUENCY, \
    }

static constexpr BuiltinSongDefinition BUILTIN_SONG_REGISTRY[] = {
    BUILTIN_SONG_ENTRY("SONG_01", "sounds/builtin_songs/song_01.h", BuiltinSongFile01),
    BUILTIN_SONG_ENTRY("SONG_02", "sounds/builtin_songs/song_02.h", BuiltinSongFile02),
    BUILTIN_SONG_ENTRY("SONG_03", "sounds/builtin_songs/song_03.h", BuiltinSongFile03),
    BUILTIN_SONG_ENTRY("SONG_04", "sounds/builtin_songs/song_04.h", BuiltinSongFile04),
    BUILTIN_SONG_ENTRY("SONG_05", "sounds/builtin_songs/song_05.h", BuiltinSongFile05),
};

#undef BUILTIN_SONG_ENTRY

static constexpr int BUILTIN_SONG_REGISTRY_COUNT =
    (int)(sizeof(BUILTIN_SONG_REGISTRY) / sizeof(BUILTIN_SONG_REGISTRY[0]));

inline const BuiltinSongDefinition *BuiltinSong_ByZeroBasedIndex(int index)
{
    if (index < 0 || index >= BUILTIN_SONG_REGISTRY_COUNT)
        return nullptr;
    return &BUILTIN_SONG_REGISTRY[index];
}

inline const BuiltinSongDefinition *BuiltinSong_BySongId(int songId)
{
    return BuiltinSong_ByZeroBasedIndex(songId - 1);
}
