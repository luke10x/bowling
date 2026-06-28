#pragma once

inline float CampaignEnemyManaCapacityScaleForAvatar(int botAvatar)
{
    switch (botAvatar)
    {
        case 1: return 2.0f; // CHERUB / Dog
        case 2: return 3.0f; // SERAPH / Beak
        case 3: return 4.0f; // THRONE / Cow
        default: return 1.0f; // ANGEL / Malach
    }
}

