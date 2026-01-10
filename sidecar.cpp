#include <iostream>

#include "sidecar.h"

#include <SDL.h>
#include <stb_image.h>

namespace acl
{
struct LoadedImageInternal
{
    LoadedImage pub; // what plugin sees
};

static LoadedImageInternal g_imageSlot; // one slot (can be expanded later)

const LoadedImage *loadImage(const char *path, bool flip)
{
    std::cerr << "Start of in acl " << path << std::endl;
    if (!path)
        return nullptr;

    stbi_set_flip_vertically_on_load(flip ? 1 : 0);

    int w = 0;
    int h = 0;
    int c = 0;

    SDL_Log("loads now %s\n", path);
    // unsigned char* data = stbi_load(path, &w, &h, &c, 0);
    //SDL_RWops *rw = SDL_RWFromFile(path, "rb");
    SDL_RWops *rw = SDL_RWFromFile(path, "rb");

    Sint64 size = SDL_RWsize(rw);
    unsigned char *buffer = new unsigned char[size];
    SDL_RWread(rw, buffer, 1, size);
    SDL_RWclose(rw);

    // int w,h,c;
    unsigned char *data = stbi_load_from_memory(buffer, size, &w, &h, &c, 4);

    if (!data)
    {
        SDL_Log("erroe");
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

void freeImage(const LoadedImage *img)
{
    if (!img || !img->data)
        return;

    // cast back to internal container
    stbi_image_free((void *)img->data);

    // reset slot
    g_imageSlot.pub.data = nullptr;
    g_imageSlot.pub.width = 0;
    g_imageSlot.pub.height = 0;
    g_imageSlot.pub.channels = 0;
}
} // namespace acl
