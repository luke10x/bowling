#pragma once
#include <xfm_song_dsl.h>

// XFM tracker song file. This is valid C++ and can be pasted into built-in songs.
XFM_SONG_BEGIN(R"xfmname(Bolt Save)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(8
PART PART 1
C-5007D|.......|.......|.......|.......|.......
G-50074|.......|.......|.......|.......|.......
.......0C02|.......|.......|.......|.......|.......
E-50062|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
OFF....|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Bolt Save
COLOR B8F7FF
      ALG FB AMS FMS
PATCH   4  6   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   18 23 18  7  0  4   0  12 -2  3  0
FM 2   33 18 10  5  0  4   0   7  1  3  0
FM 3   24 27 14  4  0  5   0   3  2  2  0
FM 4    0 25 13  3  0  7   0   2  0  2  0
ENDINST
)xfminstruments")

XFM_SONG_END()
