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
D-7007F
.......
A#6007F
.......
F#6007F
.......
C#6007F
.......
G-5007F
.......
D#5007F
.......
A-4007F
.......
F-4007F
.......
C#4007F
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
