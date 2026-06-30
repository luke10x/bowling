#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Buy)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
12
C-4007F
E-4007F
G-4007F
C-5007F
E-5007F
G-5007F
C-6007F
.......
.......
.......
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Hollow Electric
COLOR 86B7FF
PATCH 4 6 0 0
OP 1 0 3 35 0 13 0 1 25 2 0 0
OP 2 0 1 20 0 17 0 10 8 2 7 0
OP 3 0 1 11 0 8 0 4 23 7 1 0
OP 4 0 1 14 0 25 0 0 10 0 9 0
ENDINST
)xfminstruments")
XFM_SONG_END()
