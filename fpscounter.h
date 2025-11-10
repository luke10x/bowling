#pragma once

#include <iostream>
#include <iomanip>

struct FpsCounter {
    float timeAccumulator;
    int frameCount;
    float fps;

    void initFpsCounter() {
        timeAccumulator = 0.0f;
        frameCount = 0;
        fps = 0.0f;
    }

    void updateFpsCounter(float deltaTime) {
        timeAccumulator += deltaTime;
        frameCount++;

        if (timeAccumulator >= 5.0f) {
            fps = frameCount / timeAccumulator;
            std::cout << "FPS: " << std::fixed << std::setprecision(2) << fps << std::endl;
            timeAccumulator = 0.0f;
            frameCount = 0;
        }
    }
};