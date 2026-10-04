#pragma once

#include <cstddef>
#include <cstdio>

// Campaign level records are normally brief, but a stored first-win time can
// exceed an hour. Keep the familiar MM:SS form below one hour and introduce a
// real hour field above it rather than displaying values such as 73:12.
static inline void Campaign_FormatTimeToBeat(int totalSeconds, char *out, size_t outSize)
{
    if (!out || outSize == 0)
        return;
    const int safeSeconds = totalSeconds < 0 ? 0 : totalSeconds;
    const int hours = safeSeconds / 3600;
    const int minutes = (safeSeconds / 60) % 60;
    const int seconds = safeSeconds % 60;
    if (hours > 0)
        std::snprintf(out, outSize, "%d:%02d:%02d", hours, minutes, seconds);
    else
        std::snprintf(out, outSize, "%02d:%02d", minutes, seconds);
}
