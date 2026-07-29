#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Strike)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
14
C-3007F
.......
E-3007F
.......
G-3007F
.......
C-4007F
E-4007F
G-4007F
C-5007F
.......
OFF....
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Rubber Bass
COLOR 7BD88F
PATCH 2 5 0 0
OP 1 1 3 38 0 12 0 7 11 4 6 0
OP 2 -1 1 38 0 17 0 5 2 2 1 0
OP 3 1 2 5 0 11 0 13 11 5 13 0
OP 4 -1 1 0 0 31 0 9 15 5 8 3
ENDINST
)xfminstruments")
XFM_SONG_END()
