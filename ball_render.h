#pragma once

struct BallAtlasRegion
{
    float startX;
    float startY;
};

inline int BallRender_ClampCatalogId(int ballId, int catalogCount)
{
    if (catalogCount <= 0)
        return 0;
    if (ballId < 0)
        return 0;
    if (ballId >= catalogCount)
        return catalogCount - 1;
    return ballId;
}

inline BallAtlasRegion BallRender_AtlasRegionForId(int ballId, int catalogCount)
{
    const int clampedBallId = BallRender_ClampCatalogId(ballId, catalogCount);
    const float step = 1.0f / 16.0f;
    return BallAtlasRegion{
        1.0f + step * 2.0f * (float)(clampedBallId / 16),
        1.0f + step * (float)(clampedBallId % 16),
    };
}

inline int BallRender_SelectBallIdForTurn(int playerBallId, int enemyBallId, bool botMode, bool enemyTurn, int catalogCount)
{
    const int wantedBallId = (botMode && enemyTurn) ? enemyBallId : playerBallId;
    return BallRender_ClampCatalogId(wantedBallId, catalogCount);
}
