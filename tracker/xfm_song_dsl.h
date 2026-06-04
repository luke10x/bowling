#pragma once

// Small valid-C++ surface for tracker song files. The parser reads these macro
// calls directly, and the compiler can include one user song as constexpr data.
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
