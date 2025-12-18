#pragma once

#include <iostream>
#include <iomanip>

struct FpsCounter
{
    float timeAccumulator;
    int frameCount;
    float fps;

    float avgFrame;
    uint64_t NOW;
    uint64_t LAST;
    double deltaTime;
    double allTimeAccumulator;

    char fpsText[128];
    size_t fpsTextLen = 0;

    void initFpsCounter()
    {
        timeAccumulator = 0.0f;
        allTimeAccumulator = 0.0f;
        frameCount = 0;
        fps = 0.0f;

        NOW = 0;
        LAST = 0;
        this->deltaTime = 0.0;
    }

    double startFrame()
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        this->deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
        return deltaTime;
    }

    void endFrame()
    {
        timeAccumulator += deltaTime;
        frameCount++;

        uint64_t NOW2 = SDL_GetPerformanceCounter();
        // = SDL_GetTicks64();
        double frameSeconds =
            (double)(NOW2 - NOW) /
            // 1'000.0f; // because these arae in ms
            (double)SDL_GetPerformanceFrequency();

        allTimeAccumulator += frameSeconds; // will reset it now

        float measureInterval = 5.0f;
        double measuresPerSecond = 1.0 / measureInterval;
        if (timeAccumulator >= measureInterval)
        {
            fps = frameCount / timeAccumulator;

            avgFrame = allTimeAccumulator / frameCount;

            std::cout << "FPS: " << std::fixed << std::setprecision(2) << fps << std::endl;
            timeAccumulator = 0.0f;
            allTimeAccumulator = 0.0f;
            frameCount = 0;

            fpsTextLen = (size_t)snprintf(
                (char *)fpsText,
                sizeof(fpsText),
                "FPS: %.3f | Avg frame: %.3f ms",
                fps,
                avgFrame * 1000.0 // In ms
            );
        }
    }
};