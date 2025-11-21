#pragma once

#include <iostream>

#include "gl_header.h"

#include <iostream>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace vtx
{
    typedef struct
    {
        bool shouldContinue;
        SDL_Window *sdlWindow;
        SDL_GLContext sdlContext;
        int screenWidth, screenHeight;
        float pixelRatio;
        void *usrptr;
    } VertexContext;

    void openVortex(const int screenWidth, const int screenHeight);
    extern void exitVortex(int = 0);

    void init(vtx::VertexContext *ctx);
    void loop(vtx::VertexContext *ctx);
    void load(vtx::VertexContext *ctx);
    void hang(vtx::VertexContext *ctx);
}
