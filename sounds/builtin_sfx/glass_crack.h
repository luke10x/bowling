#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Glass Crack)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
8
C-8007F
F#7007F
A#7007F
D-8007F
===....
.......
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Crack
COLOR C0F0FF
PATCH 7 7 0 7
OP 1 -3 15 0 3 31 1 31 0 15 15 8
OP 2 -1 11 10 3 31 1 30 0 15 14 7
OP 3 2 13 4 3 31 1 31 0 15 15 6
OP 4 3 9 0 3 31 1 31 0 15 15 5
ENDINST
)xfminstruments")
XFM_SONG_END()
