#include "ctx.h"

#ifdef HOT_RELOAD
static LoopFunc theInit; 
static LoopFunc theLoop; 

int load_plugin()
{
    std::cerr << "trying to load loop.so" << std::endl;
    void* handle = dlopen("./build/macos/program/loop.so", RTLD_NOW | RTLD_GLOBAL);
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

    dlclose(handle);
    return 0;
}
#endif

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

