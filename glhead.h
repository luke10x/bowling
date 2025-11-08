#pragma once

#include <SDL_opengles2.h>
#include <SDL.h>

#include <GLES3/gl3.h>
#include <GL/gl3w.h>


#if defined(__EMSCRIPTEN__)
#define GLSL_VERSION "#version 300 es"
#elif defined(__ANDROID__) || defined(ANDROID)
#define GLSL_VERSION "#version 300 es"
#elif defined(TARGET_OS_OSX)
#define GLSL_VERSION "#version 330 core"
#else
#define GLSL_VERSION "#version 300 es"
#endif
