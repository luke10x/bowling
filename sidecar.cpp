#include <iostream>

#include "sidecar.h"

// stb_image implementation lives ONLY in hostlib.cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace acl
{
    struct LoadedImageInternal
    {
        LoadedImage pub; // what plugin sees
    };

    static LoadedImageInternal g_imageSlot; // one slot (can be expanded later)

    const LoadedImage* loadImage(const char* path, bool flip)
    {
        std::cerr << "Start of in acl " << path << std::endl;
        if (!path)
            return nullptr;

        stbi_set_flip_vertically_on_load(flip ? 1 : 0);

        int w = 0;
        int h = 0;
        int c = 0;

        unsigned char* data = stbi_load(path, &w, &h, &c, 0);
        if (!data)
        {
            // Failed
            g_imageSlot.pub.data = nullptr;
            g_imageSlot.pub.width = 0;
            g_imageSlot.pub.height = 0;
            g_imageSlot.pub.channels = 0;
            return nullptr;
        }

        g_imageSlot.pub.data = data;
        g_imageSlot.pub.width = w;
        g_imageSlot.pub.height = h;
        g_imageSlot.pub.channels = c;

        return &g_imageSlot.pub;
    }

    void freeImage(const LoadedImage* img)
    {
        if (!img || !img->data)
            return;

        // cast back to internal container
        stbi_image_free((void*)img->data);

        // reset slot
        g_imageSlot.pub.data = nullptr;
        g_imageSlot.pub.width = 0;
        g_imageSlot.pub.height = 0;
        g_imageSlot.pub.channels = 0;
    }
}