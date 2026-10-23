#pragma once
#include <xfm_song_dsl.h>

// XFM tracker song file. This is valid C++ and can be pasted into built-in songs.
XFM_SONG_BEGIN(R"xfmname(Glass Tinkle1)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(1)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(3)

XFM_PATTERN(R"xfmpattern(4
PART PART 1
D-70070|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
OFF....|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Glass Bell
COLOR EAFBFF
      ALG FB AMS FMS
PATCH   4  7   3   3
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1    0 20 15  8  7  1   0   4  0  3  1
FM 2    3 29 13  4  4  6   0   2  0  1  1
FM 3    7 29 23  4  6  4   0   5  1  3  0
FM 4    2 20 18  7  3  4   0   2 -1  2  1
ENDINST
INST 05
COLOR C6FF00
      ALG FB AMS FMS
PATCH   0  0   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   48 31  8 15  0  8   0   1  0  0  0
FM 2   48 31  8 15  0  8   0   1  0  0  0
FM 3   48 31  8 15  0  8   0   1  0  0  0
FM 4    0 31  8 15  0  8   0   1  0  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
