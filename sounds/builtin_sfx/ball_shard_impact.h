#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Ball Shard Impact)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
4
C#60062
REL....
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Stone Chip
COLOR CED8E0
PATCH 7 5 0 0
OP 1 -3 7 18 3 31 0 25 0 15 8 0
OP 2 2 11 10 3 31 0 22 0 15 7 0
OP 3 -1 15 8 3 31 0 21 0 15 7 0
OP 4 3 4 6 3 31 0 18 0 15 6 0
ENDINST
)xfminstruments")
XFM_SONG_END()
