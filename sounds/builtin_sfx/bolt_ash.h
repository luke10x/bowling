#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Bolt Ash)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
4
C-2007F
OFF....
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Ash Collapse
COLOR 7A8290
PATCH 0 6 0 0
OP 1 -3 14 10 3 31 0 31 0 15 5 0
OP 2 2 10 18 3 31 0 24 0 15 6 0
OP 3 -1 6 24 2 31 0 18 0 15 6 0
OP 4 0 1 0 2 31 0 13 0 15 7 0
ENDINST
)xfminstruments")
XFM_SONG_END()
