#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Ball Hit Pins)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
6
A-1007F
.......
.......
.......
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Axe
COLOR FFB86C
PATCH 4 2 3 0
OP 1 -3 1 0 0 28 1 20 24 12 15 0
OP 2 2 4 37 0 28 0 12 27 14 15 0
OP 3 -2 2 8 1 25 1 23 5 9 15 0
OP 4 3 1 6 0 27 0 25 10 2 15 0
ENDINST
)xfminstruments")
XFM_SONG_END()
