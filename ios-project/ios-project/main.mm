#include <iostream>

#define GLM_FORCE_STD140
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include "../../framework/boot.h"
#include "../../framework/boot.cpp"

#include "../../physics/physics.h"
#include "../../physics/physics.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_truetype.h>
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION

#define CLAY_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_IMPLEMENTATION

#include "../../sidecar.h"
#include "../../sidecar.cpp"
// #include "../../aurora.h"
#include "../../game.cpp"

// Aurora aura;
// void vtx::init(vtx::VertexContext *ctx) {
    // SDL_Log("Hello from vtx::init");
    // aura.initAurora();
// }

// void vtx::loop(vtx::VertexContext *ctx) {
//     //SDL_Log("Hello from vtx::loot");
//     aura.renderAurora(0.1f, glm::mat4(1.0f));
// }
void vtx::exitVortex(int exitCode)
{
}
// Compile shader helper (simplified)

extern "C" int SDL_main(int argc, char* argv[]) {
    SDL_Log("SDL + OpenGL ES 3.0 Hello World");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    // Request OpenGL ES 3.0 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create fullscreen window
    SDL_Window* window = SDL_CreateWindow(
        "Hello GLES3",
        0, 0, 0, 0,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
    );

    if (!window) {
        SDL_Log("Window failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        SDL_Log("GL context failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    SDL_Log("OpenGL ES version: %s", version);
   
    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);

    SDL_Log("OpenGL drawable size: %d x %d", width, height); 
    /* Set video details back to ctx */ {
        g_ctx.sdlContext = context;
        g_ctx.sdlWindow = window;
        g_ctx.screenWidth = width;
        g_ctx.screenHeight = height;
        g_ctx.pixelRatio = 1.0f;
    }

    printShaderVersions();

    vtx::init(&g_ctx);

    SDL_Event event;
    g_ctx.shouldContinue = true;
    while (g_ctx.shouldContinue) {
        // performOneCycle();
        g_ctx.shouldContinue = true;
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                
                g_ctx.shouldContinue = false;

            }
        }

        // Clear screen to RED

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE); // Depth write if set

        glClearColor(0.1f, 0.2f, 0.1f, 1.0f);

        vtx::loop(&g_ctx);
        checkOpenGLError("Notag");
        
        // Swap buffers
        SDL_GL_SwapWindow(window);

    }

    // Cleanup
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
