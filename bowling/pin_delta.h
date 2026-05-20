#pragma once

// bowling/pin_delta.h
// Small, dependency-free helpers for converting "cumulative pins down"
// into a per-roll delta using the same `wereDead` accumulator used by game.cpp.

// `cumulativePinsDown` is the total pins down at the end of the roll (0..10).
// `wereDead` is the running accumulator used as the previous cumulative baseline.
// Returns pins knocked this roll (can be negative if the rack was reset but `wereDead`
// was not reset to 0).
static inline int Bowling_ComputeKnockedThisRoll(int cumulativePinsDown, int wereDead)
{
    return cumulativePinsDown - wereDead;
}

// Advances the accumulator after consuming `cumulativePinsDown`.
// NOTE: This matches the current in-game logic (additive), because `wereDead`
// is reset to 0 on rack resets.
static inline void Bowling_AdvanceWereDead(int cumulativePinsDown, int *wereDead)
{
    if (!wereDead)
        return;
    *wereDead += cumulativePinsDown;
}

// Must be called whenever the rack is reset (new frame OR 10th-frame bonus rack),
// so the next roll delta is computed from a fresh rack.
static inline void Bowling_OnRackReset(int *wereDead)
{
    if (!wereDead)
        return;
    *wereDead = 0;
}

