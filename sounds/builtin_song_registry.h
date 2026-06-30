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

namespace BuiltinSongCompiled
{
#include "builtin_songs_compiled.h"
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
static constexpr const char *SONG_01_INSTRUMENTS = BuiltinSongCompiled::SONG_01_INSTRUMENTS;
static constexpr const char *SONG_01 = BuiltinSongFile01::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_02_NAME = BuiltinSongFile02::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_02_TICK_RATE = BuiltinSongFile02::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_02_SPEED = BuiltinSongFile02::XFM_TRACKER_SPEED;
static constexpr int SONG_02_ROWS_PER_BEAT = BuiltinSongFile02::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_02_SCALE_ROOT = BuiltinSongFile02::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_02_SCALE_MODE = BuiltinSongFile02::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_02_LFO_ENABLED = BuiltinSongFile02::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_02_LFO_FREQUENCY = BuiltinSongFile02::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_02_INSTRUMENTS = BuiltinSongCompiled::SONG_02_INSTRUMENTS;
static constexpr const char *SONG_02 = BuiltinSongFile02::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_03_NAME = BuiltinSongFile03::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_03_TICK_RATE = BuiltinSongFile03::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_03_SPEED = BuiltinSongFile03::XFM_TRACKER_SPEED;
static constexpr int SONG_03_ROWS_PER_BEAT = BuiltinSongFile03::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_03_SCALE_ROOT = BuiltinSongFile03::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_03_SCALE_MODE = BuiltinSongFile03::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_03_LFO_ENABLED = BuiltinSongFile03::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_03_LFO_FREQUENCY = BuiltinSongFile03::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_03_INSTRUMENTS = BuiltinSongCompiled::SONG_03_INSTRUMENTS;
static constexpr const char *SONG_03 = BuiltinSongFile03::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_04_NAME = BuiltinSongFile04::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_04_TICK_RATE = BuiltinSongFile04::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_04_SPEED = BuiltinSongFile04::XFM_TRACKER_SPEED;
static constexpr int SONG_04_ROWS_PER_BEAT = BuiltinSongFile04::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_04_SCALE_ROOT = BuiltinSongFile04::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_04_SCALE_MODE = BuiltinSongFile04::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_04_LFO_ENABLED = BuiltinSongFile04::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_04_LFO_FREQUENCY = BuiltinSongFile04::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_04_INSTRUMENTS = BuiltinSongCompiled::SONG_04_INSTRUMENTS;
static constexpr const char *SONG_04 = BuiltinSongFile04::XFM_TRACKER_SONG_PATTERN;

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

static constexpr BuiltinSongDefinition BUILTIN_SONG_REGISTRY[] = {
    {"SONG_01", "sounds/builtin_songs/song_01.h", SONG_01_NAME, SONG_01, SONG_01_INSTRUMENTS, SONG_01_TICK_RATE, SONG_01_SPEED, SONG_01_ROWS_PER_BEAT, SONG_01_SCALE_ROOT, SONG_01_SCALE_MODE, SONG_01_LFO_ENABLED != 0, SONG_01_LFO_FREQUENCY},
    {"SONG_02", "sounds/builtin_songs/song_02.h", SONG_02_NAME, SONG_02, SONG_02_INSTRUMENTS, SONG_02_TICK_RATE, SONG_02_SPEED, SONG_02_ROWS_PER_BEAT, SONG_02_SCALE_ROOT, SONG_02_SCALE_MODE, SONG_02_LFO_ENABLED != 0, SONG_02_LFO_FREQUENCY},
    {"SONG_03", "sounds/builtin_songs/song_03.h", SONG_03_NAME, SONG_03, SONG_03_INSTRUMENTS, SONG_03_TICK_RATE, SONG_03_SPEED, SONG_03_ROWS_PER_BEAT, SONG_03_SCALE_ROOT, SONG_03_SCALE_MODE, SONG_03_LFO_ENABLED != 0, SONG_03_LFO_FREQUENCY},
    {"SONG_04", "sounds/builtin_songs/song_04.h", SONG_04_NAME, SONG_04, SONG_04_INSTRUMENTS, SONG_04_TICK_RATE, SONG_04_SPEED, SONG_04_ROWS_PER_BEAT, SONG_04_SCALE_ROOT, SONG_04_SCALE_MODE, SONG_04_LFO_ENABLED != 0, SONG_04_LFO_FREQUENCY},
};

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
