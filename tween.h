#pragma once

#include <iostream>
#include <SDL.h>
#include <functional>
#include <glm/glm.hpp>
#include <algorithm> // for std::clamp

template <typename T>
struct Tween {
    enum Mode {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT
    };
    Mode mode;
    T value;                 // Current value of the tween
    T startValue;            // Starting value
    T endValue;              // Target value
    float duration = 0.0f;   // Total duration of the tween
    float elapsed = 0.0f;    // Elapsed time
    bool isActive = false;   // Is the tween active?

    // Start the tween
    void start(const T &startVal, const T &endVal, float dur, Mode mode = LINEAR) {
        this->mode = mode;
        startValue = startVal;
        endValue = endVal;
        duration = dur;
        elapsed = 0.0f;
        value = startValue;
        isActive = true;
    }

    // Update the tween
    void update(float deltaTime) {
        if (!isActive) return;

        elapsed += deltaTime;
        float t = std::clamp(elapsed / duration, 0.0f, 1.0f); // Normalised time (0 to 1)
         // Apply easing based on the mode
        switch (mode) {
            case LINEAR:
                break; // No modification needed
            case EASE_IN:
                t = t * t;
                break;
            case EASE_OUT:
                t = 1.0f - (1.0f - t) * (1.0f - t);
                break;
            case EASE_IN_OUT:
                t = t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
                break;
        }

        value = startValue + t * (endValue - startValue);     // Linear interpolation

        if (elapsed >= duration) {
            isActive = false; // Deactivate when complete
            value = endValue; // Ensure exact end value
        }
    }
};