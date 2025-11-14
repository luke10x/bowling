#include "boot.h"

// #ifndef __EMSCRIPTEN__
// #include <gl3w.c>
// #endif
// **************************
//  1. OpenGL init subsystem
// **************************
static bool initVideo(vtx::VertexContext *ctx, const int initialWidth, const int initialHeight)
{
    SDL_Init(SDL_INIT_VIDEO);

    // OpenGL ES 3 profile
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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

#ifndef __EMSCRIPTEN__
    /* Initialize gl3w - beware it cannot be included in plugin */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }
#endif

    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);

    SDL_GL_SetSwapInterval(1); // V-Sync

    std::cerr << "✅ Initial video done. Screen size: " << width << "x" << height << std::endl;

    /* Set video details back to ctx */ {
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

#ifdef HOT_RUNTIME

static bool isInitComplete = false;

#include <stdio.h>
#include <dlfcn.h>

typedef void (*LoopFunc)(vtx::VertexContext *ctx);

static LoopFunc theInit; 
static LoopFunc theLoop; 
static LoopFunc theHang; 
static LoopFunc theLoad; 

static void* pluginHandle = nullptr;
#include <sys/stat.h>
#include <ctime>

bool pluginChanged()
{
    static time_t lastWrite = 0;
    struct stat result;
    const char* file_ = TOSTRING(HOT_RUNTIME) ".log";
    if (stat(file_, &result) == 0)
    {
        if (result.st_mtime != lastWrite)
        {
            lastWrite = result.st_mtime;
            return true;
        }
    }
    return false;
}

int load_plugin()
{
    if (pluginHandle)
    {
        if (isInitComplete) {
            theHang(&g_ctx);
        }
        dlclose(pluginHandle);
        pluginHandle = nullptr;
    }
    std::cerr << "🔄 Loading " << TOSTRING(HOT_RELOAD) << "..." << std::endl;

    pluginHandle = dlopen(TOSTRING(HOT_RUNTIME), RTLD_NOW | RTLD_GLOBAL);
    if (!pluginHandle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        exit(1);
    }

    theInit = (LoopFunc)dlsym(pluginHandle, "init");
    if (!theInit)
    {
        fprintf(stderr, "dlsym init error: %s\n", dlerror());
        exit(1);
    }

    theLoop = (LoopFunc)dlsym(pluginHandle, "loop");
    if (!theLoop)
    {
        fprintf(stderr, "dlsym loop error: %s\n", dlerror());
        exit(1);
    }

    theHang = (LoopFunc)dlsym(pluginHandle, "hang");
    if (!theHang)
    {
        fprintf(stderr, "dlsym hang error: %s\n", dlerror());
        exit(1);
    }

    theLoad = (LoopFunc)dlsym(pluginHandle, "load");
    if (!theLoad)
    {
        fprintf(stderr, "dlsym load error: %s\n", dlerror());
        exit(1);
    }

    if (isInitComplete) {
        theLoad(&g_ctx);
    }

    std::cerr << "✅ Plugin loaded successfully!" << std::endl;
    return 0;
}
#endif

static void performOneCycle()
{
    g_ctx.shouldContinue = true;
#ifdef HOT_RUNTIME
    if (pluginChanged()) {
        load_plugin();
    }
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

#ifdef HOT_RUNTIME
    std::cerr << "🔥 Running in HOT RELEAD mode with swappable" << TOSTRING(HOT_RUNTIME) << std::endl;
    load_plugin();

    theInit(&g_ctx);
    isInitComplete = true;

#else
    std::cerr << "🧊 Running in STATIC build mode" << std::endl;
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

    // The End.
}

// *********************************
// Emscripten bridges 
// *********************************

#ifdef __EMSCRIPTEN__
static EM_BOOL on_web_display_size_changed(
    int event_type,
    const EmscriptenUiEvent *event,
    void *user_data)
{
    int width = event->windowInnerWidth;
    int height = event->windowInnerHeight;

    std::cerr << "web resize callback worked " << width << "x" << height
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
