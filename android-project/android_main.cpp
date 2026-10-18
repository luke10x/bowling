#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include "SDL.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "../framework/boot.h"
#include "../framework/boot.cpp"
#include "../window.h"

#include "../physics/physics.h"
#include "../physics/physics.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_truetype.h>
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION

#include "../sidecar.h"
#include "../sidecar.cpp"

#include "../../ymfm/src/ymfm_misc.cpp"
#include "../../ymfm/src/ymfm_adpcm.cpp"
#include "../../ymfm/src/ymfm_ssg.cpp"
#include "../../ymfm/src/ymfm_opn.cpp"
#include "../../eggsfm/xfm_patched_ym3438.cpp"
#include "../../eggsfm/xfm_impl.cpp"
#include "../sounds/sounds.cpp"

#include "../game.cpp"

#include "../aurora.h"


int main(int argc, char *argv[])
{
    SDL_Window* window = 0;
    SDL_GLContext gl = 0;
    if(0 != SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO))
    {
        fprintf(stderr,"Unable to initialize SDL: %s\n",SDL_GetError());
        return 1;
    }
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,    SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_DisplayMode mode;
    SDL_GetDisplayMode(0,0,&mode);
    int width = mode.w;
    int height = mode.h;
    SDL_Log("Width = %d, Heigh = %d. \n",width,height);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    window = SDL_CreateWindow(NULL,0,0,width,height,SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE);
    if(window == 0)
    {
        SDL_Log("Failed to create window.");
        SDL_Quit();
        return 1;
    }

    //Create an opengl context
    gl = SDL_GL_CreateContext(window);
    if (!gl) {
        // Handle error: SDL_GetError()
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* Set video details back to ctx */ {
        g_ctx.sdlContext = gl;
        g_ctx.sdlWindow = window;
        Android_UpdateLayoutMetrics(&g_ctx);
    }

    vtx::init(&g_ctx);

    /*Main Render Loop*/
    Uint8 done = 0;
    SDL_Event event;
    int count =  0;

    g_ctx.shouldContinue = true;
    while (g_ctx.shouldContinue) {
        // performOneCycle();
        g_ctx.shouldContinue = true;
        vtx::loop(&g_ctx);
    }
    exit(0);
}

void vtx::exitVortex(int exitCode)
{
    SDL_Quit();
    glFinish();
    if (exitCode == 0)
    {
        exit(0);
    }
    std::abort();
}
