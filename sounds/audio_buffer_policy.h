#pragma once

#include <cmath>

inline constexpr int DEFAULT_AUDIO_BUFFER_SIZE = 2048;
inline constexpr int SOUND_AUDIO_BUFFER_SIZES[] = {512, 1024, 2048, 4096, 8192};
inline constexpr double SOUND_AUDIO_BUFFER_FPS_MARGIN = 0.20;

inline int Sound_ClampAudioBufferSize(int samples)
{
    for (int size : SOUND_AUDIO_BUFFER_SIZES)
        if (samples == size)
            return size;
    return DEFAULT_AUDIO_BUFFER_SIZE;
}

// The callback interval is bufferFrames / sampleRate. Choose the smallest
// offered buffer whose interval allows a 20% margin over the measured frame
// interval, while respecting a larger size selected in the sound settings.
inline int Sound_ChooseStartupAudioBufferSize(double averageFps, int sampleRate,
                                             int requestedBufferSize)
{
    const int current = Sound_ClampAudioBufferSize(requestedBufferSize);
    if (!std::isfinite(averageFps) || averageFps <= 0.0 || sampleRate <= 0)
        return current;

    const double minimumFrames =
        (1.0 + SOUND_AUDIO_BUFFER_FPS_MARGIN) * sampleRate / averageFps;
    for (int size : SOUND_AUDIO_BUFFER_SIZES)
        if (size >= current && size >= minimumFrames)
            return size;
    return SOUND_AUDIO_BUFFER_SIZES[4];
}
