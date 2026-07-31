#pragma once

#include "score.h"

inline BowlingScoreboard stubSB;

inline void setupStubScoreboard(BowlingScoreboard *sb) {
    resetScoreboard(sb);

    addRoll(sb, 1);
    addRoll(sb, 2);

    addRoll(sb, 3);
    addRoll(sb, 4);
}

inline void setupStubScoreboardMax(BowlingScoreboard *sb) {
    resetScoreboard(sb);

    // addRoll(sb, 10);
    // addRoll(sb, 0);
    // addRoll(sb, 1);
    addRoll(sb, 10);
    addRoll(sb, 10);
    addRoll(sb, 10);

    addRoll(sb, 10);
    addRoll(sb, 10);
    addRoll(sb, 10);

    addRoll(sb, 1);
    addRoll(sb, 0);
    addRoll(sb, 10);
    addRoll(sb, 10);

    addRoll(sb, 1);
    addRoll(sb, 0);
    // addRoll(sb, 1);
    // addRoll(sb, 9);
    // addRoll(sb, 10);

    // addRoll(sb, 10);
    // addRoll(sb, 10);
    // addRoll(sb, 10);
}

inline void setupStubScoreboardFinal(BowlingScoreboard *sb) {
    resetScoreboard(sb);

    // roll 1
    addRoll(sb, 1);
    addRoll(sb, 2);

    // roll 2
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 3
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 4
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 5
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 6
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 7
    addRoll(sb, 3);
    addRoll(sb, 4);

    // Roll 8
    addRoll(sb, 3);
    addRoll(sb, 4);
}

inline void setupStubScoreboardEndOfRound(BowlingScoreboard *sb) {
    resetScoreboard(sb);

    for (int i = 0; i < 9; ++i)
    {
        addRoll(sb, 3);
        addRoll(sb, 4);
    }
}
