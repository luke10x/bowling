#pragma once
#include "../../tracker/xfm_song_dsl.h"

XFM_SONG_BEGIN(R"xfmname(Glass Tinkle)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(3)
XFM_PATTERN(R"xfmpattern(
4
D-70061
.......
OFF....
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Bell
COLOR EAFBFF
     ALG  FB AMS FMS
PATCH  4   5   3   0
FM OP TL AR DR SL SR RR SL SG ML DT RS AM
FM ID DT ML TL RS AR AM DR SR SL RR SGG
FM  1  2 13  1  2 23  1 18 24  8 12   0
FM  2 -2  3  0  2 26  1  7 22  6  7   0
FM  3  1  7  6  0 23  1  7  0 14  6   0
FM  4 -1 10  0  0 31  0  5  0 15  6   0
ENDINST
)xfminstruments")
XFM_SONG_END()
