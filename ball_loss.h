#pragma once

#include <cstdint>

namespace BallLoss
{
    static constexpr int kMaxTrackedBallId = 62;

    inline bool IsTrackableBallId(int ballId)
    {
        return ballId >= 0 && ballId <= kMaxTrackedBallId;
    }

    inline uint64_t MaskForBall(int ballId)
    {
        return IsTrackableBallId(ballId) ? (1ull << ballId) : 0ull;
    }

    inline bool RemoveDestroyedPlayerBall(uint64_t *ownedMask, uint64_t *pendingReturnMask, int ballId)
    {
        if (!ownedMask || !pendingReturnMask)
            return false;

        const uint64_t bit = MaskForBall(ballId);
        if (bit == 0ull || ((*ownedMask & bit) == 0ull))
            return false;

        *ownedMask &= ~bit;
        *pendingReturnMask |= bit;
        return true;
    }

    inline uint64_t RestorePendingDestroyedBallsOnWin(uint64_t *ownedMask, uint64_t *pendingReturnMask, bool playerWon)
    {
        if (!ownedMask || !pendingReturnMask || !playerWon)
            return 0ull;

        const uint64_t restored = *pendingReturnMask;
        *ownedMask |= restored;
        *pendingReturnMask = 0ull;
        return restored;
    }

    inline void ClearPendingDestroyedBalls(uint64_t *pendingReturnMask)
    {
        if (pendingReturnMask)
            *pendingReturnMask = 0ull;
    }
}
