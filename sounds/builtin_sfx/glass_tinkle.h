#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Glass Tinkle)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(3)
XFM_PATTERN(R"xfmpattern(
4
D-70069
.......
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Bell
COLOR EAFBFF
PATCH 4 5 3 0
OP 1 -1 12 4 3 25 1 20 16 9 10 0
OP 2 -2 3 0 2 26 1 7 22 6 7 0
OP 3 2 8 8 1 24 1 9 4 13 7 0
OP 4 -1 10 0 0 31 0 5 0 15 6 0
ENDINST
)xfminstruments")
XFM_SONG_END()
