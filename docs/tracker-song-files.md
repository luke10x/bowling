# Tracker Song Files

Tracker songs are saved as valid C++ `.h` files. The game can parse them as text for user loading, and a contributed song can also be compiled into the game.

Keep one DSL macro call per line. The text parser is intentionally line-oriented so parser errors can point at useful line numbers.

## User Song Example

```cpp
#pragma once
#include <xfm_song_dsl.h>

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

Builtin song files use this same format. They should be self-contained and include only the `XFM_INSTRUMENT` blocks referenced by that song's pattern. Editing a builtin song should copy it into the user song slot before changing the pattern or instruments.

## Parser Errors

The loader reports pattern and instrument errors together. Common diagnostics include row-count mismatches, too many channels, empty part names, missing patch fields, unknown macro targets, macro length/value-count mismatches, and unclosed instrument blocks.
