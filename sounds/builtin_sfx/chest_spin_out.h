#pragma once
#include <xfm_song_dsl.h>

XFM_SONG_BEGIN(R"xfmname(Chest Spin Out)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(12
D-5007F
D-50060
D-50042
D-50028
.......
.......
OFF....
.......
.......
.......
.......
.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Chest Spin
COLOR FFA66B
      ALG FB AMS FMS
PATCH   2  6   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   10 31 20  5  0  7   0   7 -2  2  0
FM 2   22 21 15  5  0  8   0   3  1  1  0
FM 3   34 16 10  7  0  8   0   5  0  1  0
FM 4    0 31 16  3  0  8   0   2  0  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
