#pragma once

#include <iostream>

#include "gl_header.h"

#include <iostream>

namespace vtx
{
    typedef struct
    {
        bool shouldContinue;
        SDL_Window *sdlWindow;
        SDL_GLContext sdlContext;
        int screenWidth, screenHeight;
        void* usrptr;
    } VertexContext;

    GLuint createShaderProgram(
        const char *vertexShader,
        const char *fragmentShader);
    void openVortex(const int screenWidth, const int screenHeight);
    extern void exitVortex(int = 0);
    void init(vtx::VertexContext *ctx);
    void loop(vtx::VertexContext *ctx);
}

