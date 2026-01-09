#pragma once
#include <stdint.h>

// #if defined(__ANDROID__) || defined(ANDROID)
// #include <stb_image.h>
// #endif

namespace acl
{
    struct LoadedImage
    {
        unsigned char* data;
        int width;
        int height;
        int channels;
        // host keeps ownership; plugin must *not* free this
    };

    // Returns nullptr if failed
    const LoadedImage* loadImage(const char* path, bool flip);

    // Free image (host frees it, plugin just calls)
    void freeImage(const LoadedImage* img);
}