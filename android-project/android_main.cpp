#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <memory>
#include "SDL.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "../framework/boot.h"
#include "../framework/boot.cpp"

#include "../physics/physics.h"
#include "../physics/physics.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_truetype.h>
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION

#define CLAY_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_IMPLEMENTATION

#include "../sidecar.h"
#include "../sidecar.cpp"

#include "../game.cpp"

#include "../aurora.h"


Aurora aura;

int main(int argc, char *argv[])
{
    SDL_Window* window = 0;
    SDL_GLContext gl = 0;
    if(0 != SDL_Init(SDL_INIT_VIDEO))
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

    vtx::init(&g_ctx);

    /*Main Render Loop*/
    Uint8 done = 0;
    SDL_Event event;
    int count =  0;
    while(!done)
    {
        /*Check for events*/
        SDL_Log("%d\n",count++);
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT || event.type == SDL_KEYDOWN || event.type == SDL_FINGERDOWN)
            {
                done = 1;
            }
        }

        vtx::loop(&g_ctx);
        SDL_GL_SwapWindow(window);
        SDL_Delay(10);

        std::string s = "this is anlii stl string";
        SDL_Log("%s\n", s.c_str());

    }
    exit(0);
}
// void vtx::init(vtx::VertexContext *ctx) {
//     aura.initAurora();
// }

// void vtx::loop(vtx::VertexContext *ctx) {
//     aura.renderAurora(0.0f, glm::mat4(1.0f));
// }
void vtx::exitVortex(int exitCode)
{
    // printf("GPU time elapsed AQtun: %u ns\n", usr.gpuTimeElapsed);
//    SDL_GL_DeleteContext(ctx.sdlContext);
//    SDL_DestroyWindow(ctx.sdlWindow);
    SDL_Quit();
    glFinish();
    if (exitCode == 0)
    {
        exit(0);
    }
    std::abort();
}