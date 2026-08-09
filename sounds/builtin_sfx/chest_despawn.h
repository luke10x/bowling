#pragma once
#include <xfm_song_dsl.h>

XFM_SONG_BEGIN(R"xfmname(Chest Despawn)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(1)
XFM_LFO_FREQUENCY(5)

XFM_PATTERN(R"xfmpattern(8
F-30070
.......
.......
.......
.......
OFF....
.......
.......
)xfmpattern")

XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Chest Fades
COLOR 6BA6FF
      ALG FB AMS FMS
PATCH   1  4   0   0
FM OP  TL AR DR SL SR RR SSG MUL DT RS AM
FM 1   18 30 18  6  0  8   0   1 -2  1  0
FM 2   27 20 12  5  0  8   0   2  0  1  0
FM 3   36 17 10  7  0 10   0   3  1  1  0
FM 4    0 28 18  4  0 11   0   1  0  0  0
ENDINST
)xfminstruments")

XFM_SONG_END()
