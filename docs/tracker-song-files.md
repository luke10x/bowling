# Tracker Song Files

Tracker songs are saved as valid C++ `.h` files. The game can parse them as text for user loading, and a contributed song can also be compiled into the game.

Keep one DSL macro call per line. The text parser is intentionally line-oriented so parser errors can point at useful line numbers.

## User Song Example

```cpp
#pragma once
#include "tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Example Song)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(6)
XFM_ROWS_PER_BEAT(4)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(3)

XFM_PATTERN(R"xfmpattern(4
PART Intro
C-4007F|.......|.......|.......|.......|.......
.......|E-40070|.......|.......|.......|.......
SKIP Muted sketch
D-4007F|.......|.......|.......|.......|.......
PART Chorus
G-4007F|.......|.......|.......|.......|.......
)xfmpattern")

XFM_INSTRUMENT(0x00)
XFM_INSTRUMENT_NAME("Lead")
XFM_INSTRUMENT_COLOR(0xA0B0C0)
XFM_PATCH(ALG = 4, FB = 5, AMS = 1, FMS = 2)
XFM_OP(1, DT = 0, MUL = 1, TL = 20, RS = 0, AR = 31, AM = 1, DR = 12, SR = 8, SL = 3, RR = 7, SSG = 0)
XFM_OP(2, DT = 0, MUL = 1, TL = 32, RS = 0, AR = 28, AM = 0, DR = 10, SR = 7, SL = 4, RR = 7, SSG = 0)
XFM_OP(3, DT = 0, MUL = 1, TL = 40, RS = 0, AR = 24, AM = 0, DR = 8, SR = 5, SL = 6, RR = 7, SSG = 0)
XFM_OP(4, DT = 0, MUL = 1, TL = 0, RS = 0, AR = 31, AM = 0, DR = 12, SR = 8, SL = 3, RR = 7, SSG = 0)
XFM_TRACKER_MACRO(TL1, LENGTH = 4, LOOP = 255, RELEASE = 255, VALUES = "20 24 28 32")
XFM_END_INSTRUMENT()

XFM_SONG_END()
```

`PART` and `SKIP` lines live inside the pattern text. `SKIP` parts are skipped during playback, not merely muted.

## Builtin Song Example

Builtin songs use named constants so multiple songs can be included in the same translation unit:

```cpp
#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_BUILTIN_SONG_BEGIN(SONG_01, "Bowling Strike")
XFM_BUILTIN_TICK_RATE(SONG_01, 60)
XFM_BUILTIN_SPEED(SONG_01, 6)
XFM_BUILTIN_ROWS_PER_BEAT(SONG_01, 4)
XFM_BUILTIN_LFO_ENABLED(SONG_01, 0)
XFM_BUILTIN_LFO_FREQUENCY(SONG_01, 0)

XFM_INSTRUMENT(0x00)
XFM_INSTRUMENT_NAME("Rubber Bass")
XFM_INSTRUMENT_COLOR(0x7BD88F)
XFM_PATCH(ALG = 2, FB = 5, AMS = 0, FMS = 0)
XFM_OP(1, DT = 1, MUL = 3, TL = 38, RS = 0, AR = 12, AM = 0, DR = 7, SR = 11, SL = 4, RR = 6, SSG = 0)
XFM_OP(2, DT = -1, MUL = 1, TL = 38, RS = 0, AR = 17, AM = 0, DR = 5, SR = 2, SL = 2, RR = 1, SSG = 0)
XFM_OP(3, DT = 1, MUL = 2, TL = 5, RS = 0, AR = 11, AM = 0, DR = 13, SR = 11, SL = 5, RR = 13, SSG = 0)
XFM_OP(4, DT = -1, MUL = 1, TL = 0, RS = 0, AR = 31, AM = 0, DR = 9, SR = 15, SL = 5, RR = 8, SSG = 3)
XFM_END_INSTRUMENT()

XFM_BUILTIN_PATTERN(SONG_01, R"(1
C-3007F|.......|.......|.......|.......|.......
)")
XFM_BUILTIN_SONG_END(SONG_01)
```

Builtin song files should be self-contained. Include only the `XFM_INSTRUMENT` blocks referenced by that song's pattern. Editing a builtin song should copy it into the user song slot before changing the pattern or instruments.

## Parser Errors

The loader reports pattern and instrument errors together. Common diagnostics include row-count mismatches, too many channels, empty part names, missing patch fields, unknown macro targets, macro length/value-count mismatches, and unclosed instrument blocks.
