#pragma once

// Small valid-C++ surface for tracker song files. The parser reads these macro
// calls directly, and the compiler sees the old constexpr symbols.
#define XFM_SONG_NAME(value) static constexpr const char *XFM_TRACKER_SONG_NAME = value;
#define XFM_TICK_RATE(value) static constexpr int XFM_TRACKER_TICK_RATE = value;
#define XFM_SPEED(value) static constexpr int XFM_TRACKER_SPEED = value;
#define XFM_ROWS_PER_BEAT(value) static constexpr int XFM_TRACKER_ROWS_PER_BEAT = value;
#define XFM_LFO_ENABLED(value) static constexpr int XFM_TRACKER_LFO_ENABLED = value;
#define XFM_LFO_FREQUENCY(value) static constexpr int XFM_TRACKER_LFO_FREQUENCY = value;
#define XFM_PATTERN(value) static constexpr const char *XFM_TRACKER_SONG_PATTERN = value;
#define XFM_INSTRUMENTS(value) static constexpr const char *XFM_TRACKER_CUSTOM_INSTRUMENTS = value;

#define XFM_SONG_BEGIN(value) XFM_SONG_NAME(value)
#define XFM_SONG_END()

#define XFM_INSTRUMENT(...)
#define XFM_INSTRUMENT_NAME(...)
#define XFM_INSTRUMENT_COLOR(...)
#define XFM_PATCH(...)
#define XFM_OP(...)
#define XFM_TRACKER_MACRO(...)
#define XFM_END_INSTRUMENT()

#define XFM_BUILTIN_SONG_BEGIN(symbol, display_name) static constexpr const char *symbol##_NAME = display_name;
#define XFM_BUILTIN_TICK_RATE(symbol, value) static constexpr int symbol##_TICK_RATE = value;
#define XFM_BUILTIN_SPEED(symbol, value) static constexpr int symbol##_SPEED = value;
#define XFM_BUILTIN_ROWS_PER_BEAT(symbol, value) static constexpr int symbol##_ROWS_PER_BEAT = value;
#define XFM_BUILTIN_LFO_ENABLED(symbol, value) static constexpr int symbol##_LFO_ENABLED = value;
#define XFM_BUILTIN_LFO_FREQUENCY(symbol, value) static constexpr int symbol##_LFO_FREQUENCY = value;
#define XFM_BUILTIN_INSTRUMENTS(symbol, value) static constexpr const char *symbol##_INSTRUMENTS = value;
#define XFM_BUILTIN_PATTERN(symbol, value) static constexpr const char *symbol = value;
#define XFM_BUILTIN_SONG_END(symbol)
