#pragma once

#include "../clayton/slider.h"

struct GameSettings
{
    Clayton_Slider snowflakeSlider;
    int maxSnowflakes = 0;
    int snowflakeCount = 0;

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
