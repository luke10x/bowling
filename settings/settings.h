#pragma once

#include "../clayton/slider.h"

struct GameSettings
{
    Clayton_Slider snowflakeSlider;
    int maxSnowflakes = 0;
    int snowflakeCount = 0;
    enum WebUpdateStatus
    {
        WEB_UPDATE_UNSUPPORTED = 0,
        WEB_UPDATE_IDLE,
        WEB_UPDATE_CHECKING,
        WEB_UPDATE_UP_TO_DATE,
        WEB_UPDATE_AVAILABLE,
        WEB_UPDATE_OFFLINE,
        WEB_UPDATE_ERROR,
        WEB_UPDATE_APPLYING
    };
    WebUpdateStatus webUpdateStatus = WEB_UPDATE_UNSUPPORTED;
    char installedBuild[32] = {0};
    char publishedBuild[32] = {0};
    bool webUpdateStandalone = false;

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
    }

    bool webUpdateAvailable() const
    {
        return webUpdateStatus == WEB_UPDATE_AVAILABLE;
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
};
