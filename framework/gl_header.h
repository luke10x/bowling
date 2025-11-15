#pragma once

// #include <SDL_opengles2.h>
// #include <SDL.h>
// #include <GLES3/gl3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#pragma once

// =========================
// Apple Platforms
// =========================
#if defined(__APPLE__)
    #include <TargetConditionals.h>

    // ---- iOS / tvOS ----
    #if TARGET_OS_IOS || TARGET_OS_TV
        #include <OpenGLES/ES3/gl.h>
        #include <OpenGLES/ES3/glext.h>

        #include <SDL_opengles2.h>

        #include <SDL.h>
    // ---- macOS ----
    #elif TARGET_OS_MAC
            #if defined(FORCE_CORE_PROFILE)
                #if defined(HOT_RUNTIME)
                    #error "Cannot force core profile with hot release enabled"
                #endif
                #include <SDL.h>
                #include "GL/gl3w.h"
                // #define GL_SILENCE_DEPRECATION
                // #include <OpenGL/gl3.h>
                #define GLSL_VERSION "#version 330 core"
            #else
                #include <SDL.h>
                #include <GLES3/gl3.h> 
                #define GLSL_VERSION "#version 300 es"
            #endif

    #else
        #error "Unknown Apple platform"
    #endif


// =========================
// Android
// =========================
#elif defined(__ANDROID__) || defined(ANDROID)
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>


// =========================
// Windows
// =========================
#elif defined(_WIN32)

    // Option A: Desktop OpenGL (most common with SDL/GLFW)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <GL/gl.h>

    // You will likely need a loader such as:
    // - glad
    // - GLEW
    // - gl3w
    // Because Windows only exposes OpenGL 1.1 without an extension loader.

    // Option B: If you're using ANGLE for GLES:
    // #include <GLES3/gl3.h>
    // #include <GLES3/gl3ext.h>


// =========================
// Linux
// =========================
#elif defined(__linux__)

    // Option A: Desktop GL
    #include <GL/gl.h>

    // Option B: GLES3 (Mesa EGL or ANGLE)
    // #include <GLES3/gl3.h>
    // #include <GLES3/gl3ext.h>
// =========================
// Emscripten
// =========================
#elif defined(__EMSCRIPTEN__)
    #include <emscripten.h>
    #include <emscripten/html5.h>

    #include <SDL_opengles2.h>
    #include <SDL.h>
    #include <GLES3/gl3.h>

    #define GLSL_VERSION "#version 300 es"
#else
    #error "Platform not supported"
#endif