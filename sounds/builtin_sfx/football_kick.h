#pragma once
#include "../../tracker/xfm_song_dsl.h"

// Short low thump followed by a brighter snap, like a foot striking the ball.
XFM_SONG_BEGIN(R"xfmname(Football Kick)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(3)
XFM_ROWS_PER_BEAT(1)
XFM_SCALE_ROOT(0)
XFM_SCALE_MODE(0)
XFM_LFO_ENABLED(0)
XFM_LFO_FREQUENCY(0)
XFM_PATTERN(R"xfmpattern(
7
C-3007F
G-2007F
C-4007F
OFF....
.......
.......
.......
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 00
NAME Football Kick
COLOR C5A378
PATCH 2 5 0 0
OP 1 1 3 30 0 24 0 3 11 4 6 0
OP 2 -1 1 38 0 25 0 4 2 2 1 0
OP 3 1 2 8 0 24 0 3 11 5 13 0
OP 4 -1 1 0 0 31 0 3 15 5 8 3
ENDINST
)xfminstruments")
XFM_SONG_END()
