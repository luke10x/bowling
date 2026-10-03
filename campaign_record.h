#pragma once

inline int CampaignRecord_Quits(int started, int losses, int wins)
{
    return started > losses + wins ? started - losses - wins : 0;
}

inline bool CampaignRecord_HasStartedRoll(int recordedRolls)
{
    return recordedRolls > 0;
}
