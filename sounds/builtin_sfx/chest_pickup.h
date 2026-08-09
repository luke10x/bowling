#pragma once
#include <xfm_song_dsl.h>

XFM_SONG_BEGIN(R"xfmname(Chest Pickup)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(8
PART PART 2
G-5007F|G-60038|.......|.......|.......|.......
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
NAME Chest Pickup
COLOR 9DF2D0
      ALG FB AMS FMS
PATCH   6  5   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   15 31 14  5  0  6   0   2 -1  1  0
FM 2   24 20  9  5  0  8   0   5  1  1  0
FM 3   36 18  7  6  0  8   0   7  0  1  0
FM 4    0 31 14  3  0  8   0   1  0  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
