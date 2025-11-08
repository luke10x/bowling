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
#ifdef __USE_SDL
        SDL_Window *sdlWindow;
        SDL_GLContext sdlContext;
#elif defined(__USE_GLFW)
        GLFWwindow *glfwWindow;
#endif
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


// *********************************
//  Start with initializing context
// *********************************


#include <stdio.h>
#include <dlfcn.h>

typedef void (*LoopFunc)(vtx::VertexContext *ctx);  // define a proper type for your function pointer


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
static bool initVideo(vtx::VertexContext *ctx, const int screenWidth, const int screenHeight)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    int width = 512, height = 512;
    SDL_Window* window = SDL_CreateWindow(
        "SDL GLES",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);


    /* Initialize gl3w */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    int w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    glViewport(0, 0, w, h);

    printShaderVersions();
    std::cerr << "Checking for OpenGL Error..." << std::endl;
    checkOpenGLError("INIT_VIDEO_TAG");
    std::cerr << "✅ Initial video done. Screen size: " << width << "x" << height << std::endl;
    
    SDL_GL_SetSwapInterval(1);

    ctx->sdlContext = gl_context;
    ctx->sdlWindow = window;
    ctx->screenHeight = h;
    ctx->screenWidth = w;

    return true;
}


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

