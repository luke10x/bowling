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

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Lead
COLOR A0B0C0
PATCH 4 5 1 2
OP 1 0 1 20 0 31 1 12 8 3 7 0
OP 2 0 1 32 0 28 0 10 7 4 7 0
OP 3 0 1 40 0 24 0 8 5 6 7 0
OP 4 0 1 0 0 31 0 12 8 3 7 0
MACRO 1 4 255 255 20 24 28 32
ENDINST
)xfminstruments")

XFM_SONG_END()
```

`PART` and `SKIP` lines live inside the pattern text. `SKIP` parts are skipped during playback, not merely muted.

Builtin song files use this same format. They should be self-contained and include only the `XFM_INSTRUMENTS` text referenced by that song's pattern. Editing a builtin song should copy it into the user song slot before changing the pattern or instruments.

## Parser Errors

The loader reports pattern and instrument errors together. Common diagnostics include row-count mismatches, too many channels, empty part names, missing patch fields, unknown macro targets, macro length/value-count mismatches, and unclosed instrument blocks.
