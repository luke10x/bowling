#pragma once
#include <xfm_song_dsl.h>

XFM_SONG_BEGIN(R"xfmname(Chest Spawn)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(10
PART PART 2
C-5007F|C-60044|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
OFF....|OFF....|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
.......|.......|.......|.......|.......|.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Chest Appears
COLOR FFD66B
      ALG FB AMS FMS
PATCH   5  5   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   12 31 12  4  4  6   0   2 -1  1  0
FM 2   20 24  8  5  5  7   0   3  1  1  0
FM 3   31 20  8  7  3  8   0   4  0  1  0
FM 4    0 31 10  3  4  8   0   1  0  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
