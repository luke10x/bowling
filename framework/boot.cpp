#if defined(FORCE_DESKTOP_OPENGL)
#include <gl3w.c>
#endif

#include "boot.h"
// **************************
//  1. OpenGL init subsystem
// **************************
static bool initVideo(vtx::VertexContext *ctx, const int initialWidth, const int initialHeight)
{
    SDL_Init(SDL_INIT_VIDEO);

#if defined(FORCE_DESKTOP_OPENGL)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
    // OpenGL ES 3 profile
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);


    SDL_Window* window = SDL_CreateWindow(
        "SDL GLES",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        initialWidth,
        initialHeight,
        SDL_WINDOW_OPENGL
            | SDL_WINDOW_RESIZABLE 
            | SDL_WINDOW_SHOWN
#ifndef __EMSCRIPTEN__
            | SDL_WINDOW_ALLOW_HIGHDPI
#endif
        );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	// 1  → sync to display refresh (usually 60Hz)
	// 0  → V-Sync OFF
	// -1 → “late swap tearing” (adaptive vsync, if driver supports it)
    SDL_ShowWindow(window);

    if (SDL_GL_SetSwapInterval(-1) != 0) {
        std::cerr << "Warning: VSync not available: "
                << SDL_GetError() << "\n";
    }
    SDL_Delay(1);

#if defined(FORCE_DESKTOP_OPENGL)
    /* Initialize gl3w - beware it cannot be included in plugin */
    if (gl3wInit()) {
        fprintf(stderr, "gl3w: failed to initialize\n");
        exit(EXIT_FAILURE);
    }
#endif

    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);



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
    struct stat stat_result;
    const char* file_ = TOSTRING(HOT_RUNTIME) ".log";
    if (stat(file_, &stat_result) == 0)
    {
        if (stat_result.st_mtime != lastWrite)
        {
            bool result = lastWrite != 0;
            lastWrite = stat_result.st_mtime;
            return result; // if this is first time we don't need to update
        }
    }
    return false;
}

int load_plugin()
{
    // === Store old symbol pointers before closing ===
    LoopFunc oldInit = theInit;
    LoopFunc oldLoop = theLoop;
    LoopFunc oldHang = theHang;
    LoopFunc oldLoad = theLoad;

    // === unload previous plugin ===
    if (pluginHandle)
    {
        if (isInitComplete) {
            theHang(&g_ctx);
        }
        dlclose(pluginHandle);
        pluginHandle = nullptr;
    }

    std::cerr << "🔄 Reloading plugin...\n";

    // === load new ===
    pluginHandle = dlopen(TOSTRING(HOT_RUNTIME), RTLD_NOW | RTLD_GLOBAL);
    if (!pluginHandle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        exit(1);
    }

    theInit = (LoopFunc)dlsym(pluginHandle, "init");
    theLoop = (LoopFunc)dlsym(pluginHandle, "loop");
    theHang = (LoopFunc)dlsym(pluginHandle, "hang");
    theLoad = (LoopFunc)dlsym(pluginHandle, "load");

    if (!theInit || !theLoop || !theHang || !theLoad)
    {
        fprintf(stderr, "dlsym error: %s\n", dlerror());
        exit(1);
    }

    // === detect “no effect reload” ===
    bool noEffect =
        oldInit == theInit &&
        oldLoop == theLoop &&
        oldHang == theHang &&
        oldLoad == theLoad;

    if (noEffect) {
        std::cerr << "⚠️ Plugin reloaded but symbols did NOT change (no effect)\n";
        return 2;
    }

    // === regular hot reload ===
    if (isInitComplete) {
        theLoad(&g_ctx);
    }

    std::cerr << "✅ Hot reload successful\n";
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
