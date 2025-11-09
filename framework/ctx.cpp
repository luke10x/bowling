#include "ctx.h"
#include <GL/gl3w.h>

#ifdef HOT_RELOAD
static LoopFunc theInit; 
static LoopFunc theLoop; 

int load_plugin()
{
    std::cerr << "trying to load loop.so" << std::endl;
    void* handle = dlopen("loop.so", RTLD_NOW | RTLD_GLOBAL);
    if (!handle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        exit(1);
    }

    // cast the void* from dlsym to your function pointer type
    theInit = (LoopFunc)dlsym(handle, "init");
    if (!theInit)
    {
        fprintf(stderr, "dlsym init error: %s\n", dlerror());
        exit(1);
    }
    theLoop = (LoopFunc)dlsym(handle, "loop");
    if (!theLoop)
    {
        fprintf(stderr, "dlsym loop error: %s\n", dlerror());
        exit(1);
    }

    // dlclose(handle);
    return 0;
}
#endif

static bool initVideo(vtx::VertexContext *ctx, const int initialWidth, const int initialHeight)
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "SDL GLES",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        initialWidth, initialHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    /* Initialize gl3w - beware it cannot be included in plugin */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);

    printShaderVersions();
    checkOpenGLError("INIT_VIDEO_TAG");
    std::cerr << "✅ Initial video done. Screen size: " << width << "x" << height << std::endl;
    
    SDL_GL_SetSwapInterval(1); // V-Sync

    ctx; {
        ctx->sdlContext = gl_context;
        ctx->sdlWindow = window;
        ctx->screenWidth = width;
        ctx->screenHeight = height;
    }

    return true;
}

// ************************
//  4. Main loop subsystem
// ************************

static vtx::VertexContext g_ctx;

static void performOneCycle()
{
    g_ctx.shouldContinue = true;
#ifdef HOT_RELOAD
    theLoop(&g_ctx);
#else
    vtx::loop(&g_ctx);
#endif
}

void vtx::openVortex(int screenWidth, int screenHeight)
{
    g_ctx.shouldContinue = true;

    g_ctx.screenWidth = screenWidth;
    g_ctx.screenHeight = screenHeight;

    if (!initVideo(&g_ctx, g_ctx.screenWidth, g_ctx.screenHeight))
    {
        std::cerr << "💀 Failed to initialize video!" << std::endl;
        exitVortex(1);
    }

#ifdef HOT_RELOAD
    load_plugin();

    theInit(&g_ctx);
#else
    vtx::init(&g_ctx);
#endif

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(performOneCycle, 0, 1);
#else
    while (g_ctx.shouldContinue) {
        performOneCycle();
    }
#endif

    return;

    // Should be no code beyond this point!
    // singleLoopCycle() performs the cleanup
    // when it detects that it should quit.
    // The reason for this is that in Emscripten build
    // this coude would be reached before
    // singleLoopCycle() is even called,
    // and the native code would reach here only
    // after the main loop.
    //
    // The End.
}

