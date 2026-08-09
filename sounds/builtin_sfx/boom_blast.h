#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Boom Blast)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
12
C-2007F
.......
.......
REL....
.......
OFF....
.......
.......
.......
.......
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Blast Pressure
COLOR FF7A3A
PATCH 7 7 0 0
OP 1 -3 15 4 3 31 0 31 0 15 12 8
OP 2 3 12 18 3 31 0 24 0 15 10 8
OP 3 -2 9 28 2 31 0 18 0 15 8 7
OP 4 1 1 0 2 31 0 13 0 15 7 7
ENDINST
)xfminstruments")
XFM_SONG_END()
