#pragma once
#include <xfm_song_dsl.h>

// XFM tracker song file. This is valid C++ and can be pasted into built-in songs.
XFM_SONG_BEGIN(R"xfmname(Rune Shot)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(8
PART PART 1
C-1007A0C04|...007F0C04|.......|.......|.......|.......
C-2..7F|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
C-1....|.......|.......|.......|.......|.......
.......0C00|.......|.......|.......|.......|.......
OFF....|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Rune Shot
COLOR FFFFFF
      ALG FB AMS FMS
PATCH   6  7   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1    9 14 22 11  0  0   1  15 -3  3  0
FM 2    7 12 13  6 12  3   0   1  2  3  0
FM 3    0 28  8  2  4  7   0   0  0  0  0
FM 4    5 23 24  4 10  5   1   1  3  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
