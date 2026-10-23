#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Glass Shards)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
22
.......
.......
D-8007F
.......
A#7007F
.......
F#7007F
.......
C#7007F
.......
G-6007F
.......
D#6007F
.......
A-5007F
.......
F-5007F
.......
C#5007F
===....
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Shard
COLOR EAFBFF
PATCH 7 6 0 6
OP 1 -2 8 16 3 31 1 22 0 15 12 0
OP 2 1 12 12 3 31 1 26 0 15 14 0
OP 3 3 15 8 3 31 1 24 0 15 13 0
OP 4 -3 11 2 3 31 1 20 0 15 12 0
ENDINST
)xfminstruments")
XFM_SONG_END()
