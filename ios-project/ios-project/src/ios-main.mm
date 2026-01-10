//#include <SDL2/SDL.h>
//
//#include "../../../framework/boot.h"
//#include "../../../framework/boot.cpp"
//

//
//extern "C" int main(int argc, char* argv[])
////int main(int argc, char* argv[])
//{
//    // Stub: SceneDelegate handles everything
//    return 0;
//    SDL_Log("Hello from SDL on iOS!");
//
//    SDL_Init(SDL_INIT_VIDEO);
//
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
//    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
//
//    SDL_Window* window = SDL_CreateWindow(
//        "VTX",
//        0, 0, 0, 0,
//        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
//    );
//
//    SDL_GLContext ctx = SDL_GL_CreateContext(window);
//
//    g_ctx.sdlWindow = window;
//    g_ctx.sdlContext = ctx;
//
//    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // red background
//    glClear(GL_COLOR_BUFFER_BIT);
//    SDL_GL_SwapWindow(window);
//    
//    
//    vtx::init(&g_ctx);
//
//    g_ctx.shouldContinue = true;
//    while (g_ctx.shouldContinue)
//        vtx::loop(&g_ctx);
//
//    SDL_Quit();
//    return 0;
//}
//
