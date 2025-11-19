#pragma once

#include <iostream>

#include "framework/gl_header.h"

#include "sidecar.h"

struct Texture
{
    GLuint id;
    int width;
    int height;
    acl::LoadedImage li;

    void loadTextureFromStbi(const unsigned char *stbiData, int width, int height, int nrChannels);
    void loadTextureFromData(
        const unsigned char *my_image, size_t size, const bool flip = false);
    void loadTextureFromFile(
        const char *path, const bool flip = false);
};

void Texture::loadTextureFromStbi(const unsigned char *stbiData, int width, int height, int nrChannels)
{
    GLuint hudTexture;
    glGenTextures(1, &hudTexture);
    glBindTexture(GL_TEXTURE_2D, hudTexture);

    // Set filtering to nearest (closest UV sampling)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Set wrap mode (optional, for UV coordinates outside [0,1])
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload texture data
    if (stbiData)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(
            GL_TEXTURE_2D, 0, format, width, height, 0, format,
            GL_UNSIGNED_BYTE, stbiData);
        glGenerateMipmap(GL_TEXTURE_2D);
        this->id = hudTexture;
        this->width = width;
        this->height = height;
    }
    else
    {
        std::cerr << "Failed to load texture of format: " << width << "x" << height << std::endl;
        // vtx::exitVortex(1);
        abort();
    }
}
// void Texture::loadTextureFromData(const unsigned char *my_image, size_t size, const bool flip)
// {
//     GLuint hudTexture;
//     int width, height, nrChannels;
//     stbi_set_flip_vertically_on_load(flip ? 1 : 0); // Enable vertical flip
//     unsigned char *data =
//         stbi_load_from_memory(my_image, size, &width, &height, &nrChannels, 0);
//     stbi_set_flip_vertically_on_load(flip ? 1 : 0); // Reset to default after loading

//     glGenTextures(1, &hudTexture);
//     glBindTexture(GL_TEXTURE_2D, hudTexture);

//     // Set filtering to nearest (closest UV sampling)
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

//     // Set wrap mode (optional, for UV coordinates outside [0,1])
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//     // Upload texture data
//     if (data)
//     {
//         GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
//         glTexImage2D(
//             GL_TEXTURE_2D, 0, format, width, height, 0, format,
//             GL_UNSIGNED_BYTE, data);
//         glGenerateMipmap(GL_TEXTURE_2D);
//         this->id = hudTexture;
//         this->width = width;
//         this->height = height;
//         stbi_image_free(data); // Free image data after loading to GPU
//     }
//     else
//     {
//         std::cerr << "Failed to load texture of sizebytes: " << size << std::endl;
//         // vtx::exitVortex(1);
//         abort();
//     }
// }

void Texture::loadTextureFromFile(const char *texturePath, const bool flip)
{
    GLuint hudTexture;

    std::cerr << "Start of loading image " << std::endl;
    const acl::LoadedImage *li = acl::loadImage(texturePath, flip);
    unsigned char *data = li->data;
    int width = li->width;
    int height = li->height;
    int nrChannels = li->channels;

    glGenTextures(1, &hudTexture);
    glBindTexture(GL_TEXTURE_2D, hudTexture);

    // Set filtering to nearest (closest UV sampling)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Set wrap mode (optional, for UV coordinates outside [0,1])
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Upload texture data
    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(
            GL_TEXTURE_2D, 0, format, width, height, 0, format,
            GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        this->id = hudTexture;
        this->width = width;
        this->height = height;
        acl::freeImage(li);
    }
    else
    {
        std::cerr << "Failed to load texture at: " << texturePath << std::endl;
        // vtx::exitVortex(1);
        abort();
    }
}
