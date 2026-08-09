#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Bolt Thunder)xfmname")
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
NAME Thunder Clap
COLOR FFD36D
PATCH 0 7 0 0
OP 1 -3 14 0 3 31 0 30 0 15 8 0
OP 2 2 9 12 3 31 0 25 0 15 8 0
OP 3 -1 6 20 2 31 0 21 0 15 8 0
OP 4 3 1 2 2 31 0 12 0 15 8 0
ENDINST
)xfminstruments")
XFM_SONG_END()
