#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Boom Slap)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(4)
XFM_PATTERN(R"xfmpattern(
6
C-1007F
.......
REL....
.......
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Tsh
COLOR C51162
      ALG FB AMS FMS
PATCH   0  5   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   31 22 17  4 16  7   0   2  0  0  0
FM 2    0 17 14  9 20  1   1   1  0  0  0
FM 3    3 24 15  7 17 10   1   0  0  0  0
FM 4    0 28 16  6 10  9   0   1  0  0  0
ENDINST
)xfminstruments")
XFM_SONG_END()
