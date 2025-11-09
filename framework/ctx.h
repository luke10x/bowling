#pragma once

#include <iostream>

#include "glhead.h"

#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// **********************
//  Global state context
// **********************

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

#ifndef HOT_RELOAD
    void init(vtx::VertexContext *ctx);
    void loop(vtx::VertexContext *ctx);
#endif
} // namespace vtx

// *******************************
//  Declarations of all functions
// *******************************

// 1. OpenGL init subsystem

static bool initVideo(vtx::VertexContext *ctx, const int screenWidth, const int screenHeight);

// 2. OpenGL shader subsystem
namespace vtx
{
    GLuint createShaderProgram(
        const char *vertexShader,
        const char *fragmentShader);
}
static GLuint compileShader(GLenum type, const char *source);

// 3. OpenGL diagnostic utils

static void printShaderVersions();
static void checkOpenGLError(const char *optionalTag);

// 4. Main loop subsystem

static void performOneCycle();
namespace vtx
{
    void openVortex(const int screenWidth, const int screenHeight);
    extern void exitVortex(int = 0);

} // namespace vtx

#ifdef HOT_RELOAD_PLUGIN
namespace vtx
{
    void init(vtx::VertexContext *ctx);
    void loop(vtx::VertexContext *ctx);
}
#endif

// *********************************
//  Start with initializing context
// *********************************


#include <stdio.h>
#include <dlfcn.h>

typedef void (*LoopFunc)(vtx::VertexContext *ctx);

// **************************
//  1. OpenGL init subsystem
// **************************
#ifdef __EMSCRIPTEN__
static EM_BOOL on_web_display_size_changed(
    int event_type,
    const EmscriptenUiEvent *event,
    void *user_data)
{
    int width = event->windowInnerWidth;
    int height = event->windowInnerHeight;

    std::cerr << "web rsize callback worked " << width << "x" << height
              << std::endl;
    SDL_Event resizeEvent;
    resizeEvent.type = SDL_WINDOWEVENT;
    resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
    resizeEvent.window.data1 = width;
    resizeEvent.window.data2 = height;
    SDL_PushEvent(&resizeEvent);
    return 0;
}
#endif
static bool initVideo(vtx::VertexContext *ctx, const int screenWidth, const int screenHeight);

// ****************************
//  3. OpenGL diagnostic utils
// ****************************

static void printShaderVersions()
{
    // Get OpenGL version
    const GLubyte *glVersion = glGetString(GL_VERSION);
    printf("OpenGL Version       : %s\n", glVersion);
    // Get GLSL (shader language) version
    const GLubyte *glslVersion =
        glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GLSL (Shader) Version: %s\n", glslVersion);
    // You can also get OpenGL vendor and renderer information
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    printf("Vendor               : %s\n", vendor);
    printf("Renderer             : %s\n", renderer);
#if defined(__ANDROID__) || defined(ANDROID)
#else
    GLint maxVertexUniformComponents;
    GLint maxVertexUniformVectors;
    GLint maxUniformBlockSize;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxVertexUniformVectors);
    // glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    printf("GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB     : %d\n", maxVertexUniformComponents); // * 4 bytes
    printf("GL_MAX_VERTEX_UNIFORM_VECTORS            : %d\n", maxVertexUniformVectors);    // * 4 bytes
    printf("GL_MAX_UNIFORM_BLOCK_SIZE                : %d\n", maxUniformBlockSize);        // * 4 bytes
#endif
}

static void checkOpenGLError(const char *optionalTag = "")
{
    bool found = false;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error optionalTag=" << optionalTag << " errorCode=" << err
                  << std::endl;

        found = true;
    }
    if (found)
    {
        // vtx::exitVortex();
        abort();
    }
}

