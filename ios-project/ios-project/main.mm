#include <iostream>
#include <SDL_syswm.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_STD140
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLES_SILENCE_DEPRECATION

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

#include "../../sidecar.h"
#include "../../sidecar.cpp"

#include "../../../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_misc.cpp"
#include "../../../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_adpcm.cpp"
#include "../../../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_ssg.cpp"
#include "../../../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_opn.cpp"
#include "../../../eggsfm/xfm_impl.cpp"
#include "../../sounds/sounds.cpp"

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
    SDL_Log("IOS_GL_DIAG_BUILD_V2");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) < 0) {
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

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Create fullscreen window
    SDL_Window* window = SDL_CreateWindow(
        "Hello GLES3",
        0, 0, 0, 0,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
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
    if (SDL_GL_MakeCurrent(window, context) != 0) {
        SDL_Log("SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_SetSwapInterval(1);
    SDL_PumpEvents();

    const GLubyte* version = glGetString(GL_VERSION);
    SDL_Log("OpenGL ES version: %s", version);

    int winWidth, winHeight;
    SDL_GetWindowSize(window, &winWidth, &winHeight);

    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);
    glViewport(0, 0, width, height);

    const float pixelRatio = winWidth > 0 ? (float)width / (float)winWidth : 1.0f;
    SDL_Log("OpenGL window size: %d x %d", winWidth, winHeight);
    SDL_Log("OpenGL drawable size: %d x %d pixelRatio=%.2f", width, height, pixelRatio);
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        setDefaultOpenGLFramebuffer(wmInfo.info.uikit.framebuffer, wmInfo.info.uikit.colorbuffer);
        bindDefaultOpenGLFramebuffer();
        SDL_Log(
            "UIKit framebuffer=%u colorbuffer=%u resolveFramebuffer=%u",
            wmInfo.info.uikit.framebuffer,
            wmInfo.info.uikit.colorbuffer,
            wmInfo.info.uikit.resolveFramebuffer
        );
    } else {
        SDL_Log("SDL_GetWindowWMInfo failed: %s", SDL_GetError());
    }
#endif
    SDL_Log("Default framebuffer before game init: 0x%x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    /* Set video details back to ctx */ {
        g_ctx.sdlContext = context;
        g_ctx.sdlWindow = window;
        g_ctx.screenWidth = winWidth;
        g_ctx.screenHeight = winHeight;
        g_ctx.pixelRatio = pixelRatio;
    }

    printShaderVersions();

    vtx::init(&g_ctx);

    g_ctx.shouldContinue = true;
    while (g_ctx.shouldContinue) {
        g_ctx.shouldContinue = true;
        vtx::loop(&g_ctx);
    }

    // Cleanup
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
