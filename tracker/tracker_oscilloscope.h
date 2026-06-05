#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

static constexpr int TRACKER_OSC_CHANNELS = 6;
static constexpr int TRACKER_OSC_ATLAS_WIDTH = 1024;
static constexpr int TRACKER_OSC_ATLAS_HEIGHT = 768;
static constexpr int TRACKER_OSC_RING_SIZE = 8192;

struct TrackerOscilloscopeChannelSnapshot
{
    const int16_t *ring = nullptr;
    int ringSize = 0;
    uint32_t writeIndex = 0;
    uint64_t sampleCursor = 0;
    uint64_t noteStartSample = 0;
    int fnum = 0;
    int block = 0;
    bool keyOn = false;
};

struct TrackerOscilloscopeSnapshot
{
    TrackerOscilloscopeChannelSnapshot channels[TRACKER_OSC_CHANNELS] = {};
    int sampleRate = 44100;
};

inline uint32_t TrackerOscilloscope_Rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

inline float TrackerOscilloscope_OpnFrequencyHz(int fnum, int block)
{
    if (fnum <= 0 || block < 0 || block > 7) return 0.0f;
    return ((float)fnum * 144.0f) / std::pow(2.0f, (float)(20 - block));
}

inline float TrackerOscilloscope_PeriodSamples(int sampleRate, int fnum, int block)
{
    float hz = TrackerOscilloscope_OpnFrequencyHz(fnum, block);
    if (hz <= 0.0f || sampleRate <= 0) return 0.0f;
    return (float)sampleRate / hz;
}

inline int TrackerOscilloscope_FastWrap(int value, int modulo)
{
    if (modulo <= 1) return 0;
    value %= modulo;
    return value < 0 ? value + modulo : value;
}

inline void TrackerOscilloscope_SetPixel(uint32_t *pixels, int width, int height, int x, int y, uint32_t color)
{
    if (!pixels || x < 0 || y < 0 || x >= width || y >= height) return;
    pixels[y * width + x] = color;
}

inline void TrackerOscilloscope_DrawLine(uint32_t *pixels, int width, int height, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;)
    {
        TrackerOscilloscope_SetPixel(pixels, width, height, x0, y0, color);
        TrackerOscilloscope_SetPixel(pixels, width, height, x0, y0 + 1, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

inline int16_t TrackerOscilloscope_ReadRingSample(const TrackerOscilloscopeChannelSnapshot &ch, uint64_t absoluteSample)
{
    if (!ch.ring || ch.ringSize <= 0) return 0;
    return ch.ring[absoluteSample & (uint64_t)(ch.ringSize - 1)];
}

inline void TrackerOscilloscope_DrawChannel(
    uint32_t *pixels,
    int width,
    int height,
    const TrackerOscilloscopeChannelSnapshot &ch,
    int sampleRate,
    int channelIndex
)
{
    if (!pixels || width <= 0 || height <= 0 || channelIndex < 0 || channelIndex >= TRACKER_OSC_CHANNELS)
        return;

    int rowH = std::max(1, height / TRACKER_OSC_CHANNELS);
    int y0 = channelIndex * rowH;
    int y1 = channelIndex == TRACKER_OSC_CHANNELS - 1 ? height : y0 + rowH;
    int midY = y0 + (y1 - y0) / 2;
    int usableH = std::max(2, y1 - y0 - 10);
    uint32_t grid = TrackerOscilloscope_Rgba(24, 42, 52, 255);
    uint32_t wave = TrackerOscilloscope_Rgba(103, 236, 212, 255);
    uint32_t quiet = TrackerOscilloscope_Rgba(64, 92, 104, 255);

    for (int x = 0; x < width; x += 64)
        TrackerOscilloscope_DrawLine(pixels, width, height, x, y0 + 4, x, y1 - 5, grid);
    TrackerOscilloscope_DrawLine(pixels, width, height, 0, midY, width - 1, midY, grid);

    float periodF = TrackerOscilloscope_PeriodSamples(sampleRate, ch.fnum, ch.block);
    int period = std::max(1, (int)std::round(periodF));
    bool silent = !ch.keyOn || !ch.ring || ch.ringSize <= 0 || period <= 1 || ch.sampleCursor == 0;
    if (silent)
    {
        TrackerOscilloscope_DrawLine(pixels, width, height, 0, midY, width - 1, midY, quiet);
        return;
    }

    uint64_t latest = ch.sampleCursor > 0 ? ch.sampleCursor - 1 : 0;
    uint64_t start = ch.noteStartSample <= latest ? ch.noteStartSample : latest;
    uint64_t sinceStart = latest - start;
    uint64_t cycle = period > 0 ? sinceStart / (uint64_t)period : 0;
    uint64_t anchor = start + (cycle * (uint64_t)period);
    if (anchor + (uint64_t)std::min(width - 1, period - 1) > latest && cycle > 0)
        anchor -= (uint64_t)period;

    int prevX = 0;
    int prevY = midY;
    bool havePrev = false;
    int activity = 0;
    for (int x = 0; x < width; x++)
    {
        uint64_t sample = anchor + (uint64_t)TrackerOscilloscope_FastWrap(x, period);
        int16_t v = TrackerOscilloscope_ReadRingSample(ch, sample);
        activity = std::max(activity, std::abs((int)v));
        float normal = std::max(-1.0f, std::min(1.0f, (float)v / 32768.0f));
        int y = midY - (int)std::round(normal * (float)usableH * 12.5f);
        y = std::max(y0 + 4, std::min(y1 - 5, y));
        if (havePrev)
            TrackerOscilloscope_DrawLine(pixels, width, height, prevX, prevY, x, y, wave);
        prevX = x;
        prevY = y;
        havePrev = true;
    }

    if (activity < 10)
        TrackerOscilloscope_DrawLine(pixels, width, height, 0, midY, width - 1, midY, quiet);
}

inline void TrackerOscilloscope_DrawAtlas(
    uint32_t *screen_pixels,
    int width,
    int height,
    const TrackerOscilloscopeSnapshot &snapshot
)
{
    if (!screen_pixels || width <= 0 || height <= 0) return;
    std::fill(screen_pixels, screen_pixels + (width * height), TrackerOscilloscope_Rgba(0, 0, 0, 255));
    for (int ch = 0; ch < TRACKER_OSC_CHANNELS; ch++)
        TrackerOscilloscope_DrawChannel(screen_pixels, width, height, snapshot.channels[ch], snapshot.sampleRate, ch);
}
