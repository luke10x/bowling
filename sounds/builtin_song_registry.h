#pragma once

#include <cstddef>

namespace BuiltinSongFileGutterGroove
{
#include "builtin_songs/gutter_groove.h"
}

namespace BuiltinSongFileAlleyCat
{
#include "builtin_songs/alley_cat.h"
}

namespace BuiltinSongFilePensativeBall
{
#include "builtin_songs/pensative_ball.h"
}

namespace BuiltinSongFilePinCrusher
{
#include "builtin_songs/pin_crusher.h"
}

namespace BuiltinSongFileEmpty
{
#include "builtin_songs/empty.h"
}

// Legacy aliases kept so the rest of the codebase can migrate gradually while
// built-in songs now come from the user-facing DSL source files directly.
static constexpr const char *SONG_01_NAME = BuiltinSongFileGutterGroove::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_01_TICK_RATE = BuiltinSongFileGutterGroove::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_01_SPEED = BuiltinSongFileGutterGroove::XFM_TRACKER_SPEED;
static constexpr int SONG_01_ROWS_PER_BEAT = BuiltinSongFileGutterGroove::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_01_SCALE_ROOT = BuiltinSongFileGutterGroove::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_01_SCALE_MODE = BuiltinSongFileGutterGroove::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_01_TUNING_MODE = BuiltinSongFileGutterGroove::XFM_TRACKER_TUNING_MODE;
static constexpr int SONG_01_LFO_ENABLED = BuiltinSongFileGutterGroove::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_01_LFO_FREQUENCY = BuiltinSongFileGutterGroove::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_01_INSTRUMENTS = BuiltinSongFileGutterGroove::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_01 = BuiltinSongFileGutterGroove::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_02_NAME = BuiltinSongFileAlleyCat::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_02_TICK_RATE = BuiltinSongFileAlleyCat::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_02_SPEED = BuiltinSongFileAlleyCat::XFM_TRACKER_SPEED;
static constexpr int SONG_02_ROWS_PER_BEAT = BuiltinSongFileAlleyCat::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_02_SCALE_ROOT = BuiltinSongFileAlleyCat::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_02_SCALE_MODE = BuiltinSongFileAlleyCat::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_02_TUNING_MODE = BuiltinSongFileAlleyCat::XFM_TRACKER_TUNING_MODE;
static constexpr int SONG_02_LFO_ENABLED = BuiltinSongFileAlleyCat::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_02_LFO_FREQUENCY = BuiltinSongFileAlleyCat::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_02_INSTRUMENTS = BuiltinSongFileAlleyCat::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_02 = BuiltinSongFileAlleyCat::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_03_NAME = BuiltinSongFilePensativeBall::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_03_TICK_RATE = BuiltinSongFilePensativeBall::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_03_SPEED = BuiltinSongFilePensativeBall::XFM_TRACKER_SPEED;
static constexpr int SONG_03_ROWS_PER_BEAT = BuiltinSongFilePensativeBall::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_03_SCALE_ROOT = BuiltinSongFilePensativeBall::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_03_SCALE_MODE = BuiltinSongFilePensativeBall::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_03_TUNING_MODE = BuiltinSongFilePensativeBall::XFM_TRACKER_TUNING_MODE;
static constexpr int SONG_03_LFO_ENABLED = BuiltinSongFilePensativeBall::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_03_LFO_FREQUENCY = BuiltinSongFilePensativeBall::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_03_INSTRUMENTS = BuiltinSongFilePensativeBall::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_03 = BuiltinSongFilePensativeBall::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_04_NAME = BuiltinSongFilePinCrusher::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_04_TICK_RATE = BuiltinSongFilePinCrusher::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_04_SPEED = BuiltinSongFilePinCrusher::XFM_TRACKER_SPEED;
static constexpr int SONG_04_ROWS_PER_BEAT = BuiltinSongFilePinCrusher::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_04_SCALE_ROOT = BuiltinSongFilePinCrusher::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_04_SCALE_MODE = BuiltinSongFilePinCrusher::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_04_TUNING_MODE = BuiltinSongFilePinCrusher::XFM_TRACKER_TUNING_MODE;
static constexpr int SONG_04_LFO_ENABLED = BuiltinSongFilePinCrusher::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_04_LFO_FREQUENCY = BuiltinSongFilePinCrusher::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_04_INSTRUMENTS = BuiltinSongFilePinCrusher::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_04 = BuiltinSongFilePinCrusher::XFM_TRACKER_SONG_PATTERN;

static constexpr const char *SONG_05_NAME = BuiltinSongFileEmpty::XFM_TRACKER_SONG_NAME;
static constexpr int SONG_05_TICK_RATE = BuiltinSongFileEmpty::XFM_TRACKER_TICK_RATE;
static constexpr int SONG_05_SPEED = BuiltinSongFileEmpty::XFM_TRACKER_SPEED;
static constexpr int SONG_05_ROWS_PER_BEAT = BuiltinSongFileEmpty::XFM_TRACKER_ROWS_PER_BEAT;
static constexpr int SONG_05_SCALE_ROOT = BuiltinSongFileEmpty::XFM_TRACKER_SCALE_ROOT;
static constexpr int SONG_05_SCALE_MODE = BuiltinSongFileEmpty::XFM_TRACKER_SCALE_MODE;
static constexpr int SONG_05_TUNING_MODE = BuiltinSongFileEmpty::XFM_TRACKER_TUNING_MODE;
static constexpr int SONG_05_LFO_ENABLED = BuiltinSongFileEmpty::XFM_TRACKER_LFO_ENABLED;
static constexpr int SONG_05_LFO_FREQUENCY = BuiltinSongFileEmpty::XFM_TRACKER_LFO_FREQUENCY;
static constexpr const char *SONG_05_INSTRUMENTS = BuiltinSongFileEmpty::XFM_TRACKER_CUSTOM_INSTRUMENTS;
static constexpr const char *SONG_05 = BuiltinSongFileEmpty::XFM_TRACKER_SONG_PATTERN;

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
    int tuningMode;
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
        ns::XFM_TRACKER_TUNING_MODE, \
        ns::XFM_TRACKER_LFO_ENABLED != 0, \
        ns::XFM_TRACKER_LFO_FREQUENCY, \
    }

static constexpr BuiltinSongDefinition BUILTIN_SONG_REGISTRY[] = {
    BUILTIN_SONG_ENTRY("GUTTER_GROOVE", "sounds/builtin_songs/gutter_groove.h", BuiltinSongFileGutterGroove),
    BUILTIN_SONG_ENTRY("ALLEY_CAT", "sounds/builtin_songs/alley_cat.h", BuiltinSongFileAlleyCat),
    BUILTIN_SONG_ENTRY("PENSATIVE_BALL", "sounds/builtin_songs/pensative_ball.h", BuiltinSongFilePensativeBall),
    BUILTIN_SONG_ENTRY("PIN_CRUSHER", "sounds/builtin_songs/pin_crusher.h", BuiltinSongFilePinCrusher),
    BUILTIN_SONG_ENTRY("EMPTY", "sounds/builtin_songs/empty.h", BuiltinSongFileEmpty),
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
