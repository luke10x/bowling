#pragma once

static inline float Campaign_EndgameBufForLevel(int levelIndex)
{
    switch (levelIndex)
    {
        case 9:  return 1.10f;
        case 10: return 1.25f;
        case 11: return 1.50f;
        case 12: return 1.75f;
        case 13: return 2.00f;
        default: return 1.00f;
    }
}

