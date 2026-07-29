#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Glass Scrape)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)
XFM_PATTERN(R"xfmpattern(
18
.......
B-6007F
A#6007F
G-6007F
F#6007F
E-6007F
D#6007F
C#6007F
B-5007F
A-5007F
G#5007F
F-5007F
E-5007F
REL....
.......
.......
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Scrape
COLOR 9EE6FF
PATCH 5 7 0 5
OP 1 -3 14 18 3 31 1 12 18 4 11 3
OP 2 3 10 8 3 31 1 16 20 5 12 4
OP 3 -1 7 12 3 31 1 18 22 6 12 5
OP 4 2 15 0 3 31 1 10 16 5 13 6
ENDINST
)xfminstruments")
XFM_SONG_END()
