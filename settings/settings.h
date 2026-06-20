#pragma once

#include "../clayton/slider.h"
#include <cstdio>
#include <cstring>

#if __has_include("../build/generated/build_version.h")
#include "../build/generated/build_version.h"
#else
#define BOWLING_BUILD_VERSION "dev"
#endif

enum GamePwaUpdateStatus
{
    GamePwaUpdateStatus_Hidden = 0,
    GamePwaUpdateStatus_Idle,
    GamePwaUpdateStatus_Checking,
    GamePwaUpdateStatus_UpToDate,
    GamePwaUpdateStatus_Available,
    GamePwaUpdateStatus_Offline,
    GamePwaUpdateStatus_Error,
    GamePwaUpdateStatus_Unsupported,
};

struct GameSettings
{
    Clayton_Slider snowflakeSlider;
    int maxSnowflakes = 0;
    int snowflakeCount = 0;
    bool pwaUpdateVisible = false;
    bool pwaUpdateCheckRequested = false;
    bool pwaUpdateApplyRequested = false;
    GamePwaUpdateStatus pwaUpdateStatus = GamePwaUpdateStatus_Hidden;
    char currentBuildVersion[32] = {};
    char latestBuildVersion[32] = {};

    void initSettings(int maxSnowflakeCount, int initialSnowflakeCount)
    {
        maxSnowflakes = maxSnowflakeCount < 0 ? 0 : maxSnowflakeCount;
        snowflakeCount = initialSnowflakeCount;
        if (snowflakeCount < 0)
            snowflakeCount = 0;
        if (snowflakeCount > maxSnowflakes)
            snowflakeCount = maxSnowflakes;

        ClaytonSlider_Init(
            &snowflakeSlider,
            "game_settings_snowflakes",
            0.0f,
            (float)maxSnowflakes,
            (float)snowflakeCount
        );

        std::snprintf(currentBuildVersion, sizeof(currentBuildVersion), "%s", BOWLING_BUILD_VERSION);
#ifdef __EMSCRIPTEN__
        pwaUpdateVisible = true;
        pwaUpdateStatus = GamePwaUpdateStatus_Idle;
#else
        pwaUpdateVisible = false;
        pwaUpdateStatus = GamePwaUpdateStatus_Hidden;
#endif
        latestBuildVersion[0] = '\0';
        pwaUpdateCheckRequested = false;
        pwaUpdateApplyRequested = false;
    }

    void syncSnowflakeCountFromSlider()
    {
        int rounded = (int)(snowflakeSlider.value + 0.5f);
        if (rounded < 0)
            rounded = 0;
        if (rounded > maxSnowflakes)
            rounded = maxSnowflakes;
        snowflakeCount = rounded;
        ClaytonSlider_SetValue(&snowflakeSlider, (float)rounded);
    }

    void setPwaUpdateStatus(GamePwaUpdateStatus status, const char *latestVersion = nullptr)
    {
        pwaUpdateStatus = status;
        if (latestVersion)
        {
            std::snprintf(latestBuildVersion, sizeof(latestBuildVersion), "%s", latestVersion);
        }
        else if (status == GamePwaUpdateStatus_Idle || status == GamePwaUpdateStatus_Checking)
        {
            latestBuildVersion[0] = '\0';
        }
    }

    bool canApplyPwaUpdate() const
    {
        return pwaUpdateVisible && pwaUpdateStatus == GamePwaUpdateStatus_Available;
    }
};
