#import "SceneDelegate.h"
#import "SDL.h"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
willConnectToSession:(UISceneSession *)session
       options:(UISceneConnectionOptions *)connectionOptions {

    if (![scene isKindOfClass:[UIWindowScene class]]) return;

    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];

    // SDL/OpenGL initialization
    // You can pass the native window pointer to SDL
    // Before creating the window
    SDL_SetMainReady(); // already called

    // Request OpenGL ES 3.0
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    // Now create the SDL window from UIWindow
    SDL_Window *sdlWindow = SDL_CreateWindowFrom((__bridge void *)self.window);
    if (!sdlWindow) {
        NSLog(@"Failed to create SDL window: %s", SDL_GetError());
        return;
    }

    // Create the context
    SDL_GLContext glContext = SDL_GL_CreateContext(sdlWindow);
    if (!glContext) {
        NSLog(@"Failed to create GL context: %s", SDL_GetError());
        return;
    }

    // Optional: check version
    int major, minor;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    NSLog(@"OpenGL ES version %d.%d", major, minor);

    // Show window
    [self.window makeKeyAndVisible];

    // Run your game loop in background thread (simple version)
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        //extern void vtx::init(SDL_Window **);
        //extern void vtx::loop();
        //vtx::init(&sdlWindow);
        while (true) {
            //vtx::loop();
            SDL_GL_SwapWindow(sdlWindow);
        }
    });
}

@end
