#include <iostream>
#include <SDL_syswm.h>
#import <UIKit/UIKit.h>
#import <GoogleMobileAds/GoogleMobileAds.h>

static void TrackerIosLossAdsStart(void);

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

#include "../../../ymfm/src/ymfm_misc.cpp"
#include "../../../ymfm/src/ymfm_adpcm.cpp"
#include "../../../ymfm/src/ymfm_ssg.cpp"
#include "../../../ymfm/src/ymfm_opn.cpp"
#include "../../../eggsfm/xfm_patched_ym3438.cpp"
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

    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
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
    float safeAreaTop = 0.0f;
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
        if (@available(iOS 11.0, *)) {
            safeAreaTop = (float)wmInfo.info.uikit.window.safeAreaInsets.top;
            SDL_Log("UIKit safe area top: %.1f", safeAreaTop);
        }
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
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
        g_ctx.iosSafeAreaTop = safeAreaTop;
#else
        g_ctx.iosSafeAreaTop = 0.0f;
#endif
    }

    vtx::init(&g_ctx);
    TrackerIosLossAdsStart();

    g_ctx.shouldContinue = true;
    while (g_ctx.shouldContinue) {
        g_ctx.shouldContinue = true;
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
        if (@available(iOS 11.0, *)) {
            SDL_SysWMinfo frameWmInfo;
            SDL_VERSION(&frameWmInfo.version);
            if (SDL_GetWindowWMInfo(window, &frameWmInfo)) {
                g_ctx.iosSafeAreaTop = (float)frameWmInfo.info.uikit.window.safeAreaInsets.top;
            }
        }
#endif
        vtx::loop(&g_ctx);
    }

    // Cleanup
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

using TrackerSongLoadedCallback = void (*)(const char *, const char *);
using TrackerSongClosedCallback = void (*)(void);
using TrackerSongSavedCallback = void (*)(int);
static TrackerSongLoadedCallback g_songLoaded = nullptr;
static TrackerSongClosedCallback g_songClosed = nullptr;
static TrackerSongSavedCallback g_songSaved = nullptr;

extern "C" void Tracker_NativeSetSongFileCallbacks(
    TrackerSongLoadedCallback loaded,
    TrackerSongClosedCallback closed,
    TrackerSongSavedCallback saved)
{
    g_songLoaded = loaded;
    g_songClosed = closed;
    g_songSaved = saved;
}

@interface TrackerSongDocumentPickerDelegate : NSObject <UIDocumentPickerDelegate>
@end

@implementation TrackerSongDocumentPickerDelegate
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
    (void)controller;
    if (g_songClosed) g_songClosed();
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
    (void)controller;
    NSURL *url = urls.firstObject;
    if (!url) {
        if (g_songClosed) g_songClosed();
        return;
    }
    BOOL accessed = [url startAccessingSecurityScopedResource];
    NSError *error = nil;
    NSString *contents = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:&error];
    if (accessed)
        [url stopAccessingSecurityScopedResource];
    if (!contents) {
        if (g_songLoaded) g_songLoaded(url.lastPathComponent.UTF8String, nullptr);
        return;
    }
    if (g_songLoaded) g_songLoaded(url.lastPathComponent.UTF8String, contents.UTF8String);
}
@end

static TrackerSongDocumentPickerDelegate *g_trackerSongDocumentPickerDelegate = nil;

static UIViewController *TrackerTopViewController(void)
{
    UIWindow *window = UIApplication.sharedApplication.keyWindow;
    if (!window) {
        for (UIWindow *candidate in UIApplication.sharedApplication.windows) {
            if (candidate.isKeyWindow) { window = candidate; break; }
        }
    }
    UIViewController *controller = window.rootViewController;
    while (controller.presentedViewController)
        controller = controller.presentedViewController;
    return controller;
}

@interface TrackerLossAdManager : NSObject <GADFullScreenContentDelegate>
@property(nonatomic, strong) GADInterstitialAd *ad;
@property(nonatomic, assign) BOOL loading;
@property(nonatomic, assign) BOOL presenting;
@property(nonatomic, assign) NSTimeInterval loadedAt;
+ (instancetype)shared;
- (void)start;
- (void)showAfterLoss;
@end

@implementation TrackerLossAdManager
+ (instancetype)shared
{
    static TrackerLossAdManager *manager;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ manager = [TrackerLossAdManager new]; });
    return manager;
}

- (void)loadAd
{
    if (self.ad || self.loading) return;
    self.loading = YES;
    NSString *unitID = [NSBundle.mainBundle objectForInfoDictionaryKey:@"LossInterstitialAdUnitID"];
    if (!unitID.length) { self.loading = NO; return; }
    [GADInterstitialAd loadWithAdUnitID:unitID
                             request:[GADRequest request]
                   completionHandler:^(GADInterstitialAd *ad, NSError *error) {
        self.loading = NO;
        if (error) {
            NSLog(@"Loss interstitial unavailable: %@", error.localizedDescription);
            return;
        }
        self.ad = ad;
        self.loadedAt = NSDate.date.timeIntervalSince1970;
        ad.fullScreenContentDelegate = self;
    }];
}

- (void)start
{
    [GADMobileAds.sharedInstance startWithCompletionHandler:nil];
    [self loadAd];
}

- (void)showAfterLoss
{
    // Google's loaded interstitials expire after one hour. Never hold up results
    // while waiting for an ad or present it later over an unrelated screen.
    if (self.ad && NSDate.date.timeIntervalSince1970 - self.loadedAt > 3600.0) {
        self.ad = nil;
    }
    UIViewController *presenter = TrackerTopViewController();
    if (!self.ad || !presenter || self.presenting) {
        [self loadAd];
        return;
    }
    self.presenting = YES;
    [self.ad presentFromRootViewController:presenter];
}

- (void)adDidDismissFullScreenContent:(id<GADFullScreenPresentingAd>)ad
{
    (void)ad;
    Game_SetAdAudioPaused(0);
    self.presenting = NO;
    self.ad = nil;
    [self loadAd];
}

- (void)adWillPresentFullScreenContent:(id<GADFullScreenPresentingAd>)ad
{
    (void)ad;
    Game_SetAdAudioPaused(1);
}

- (void)ad:(id<GADFullScreenPresentingAd>)ad didFailToPresentFullScreenContentWithError:(NSError *)error
{
    (void)ad;
    Game_SetAdAudioPaused(0);
    self.presenting = NO;
    self.ad = nil;
    NSLog(@"Loss interstitial could not open: %@", error.localizedDescription);
    [self loadAd];
}
@end

static void TrackerIosLossAdsStart(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{ [[TrackerLossAdManager shared] start]; });
}

extern "C" void PlatformAds_ShowOnLoss(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{ [[TrackerLossAdManager shared] showAfterLoss]; });
}

extern "C" void Tracker_NativeOpenSongFile(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController *presenter = TrackerTopViewController();
        if (!presenter) {
            if (g_songClosed) g_songClosed();
            return;
        }
        g_trackerSongDocumentPickerDelegate = [TrackerSongDocumentPickerDelegate new];
        UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc]
            initWithDocumentTypes:@[@"public.plain-text", @"public.source-code"]
            inMode:UIDocumentPickerModeImport];
        picker.delegate = g_trackerSongDocumentPickerDelegate;
        [presenter presentViewController:picker animated:YES completion:nil];
    });
}

extern "C" void Tracker_NativeSaveSongFile(const char *filename, const char *text)
{
    NSString *requestedName = filename ? [NSString stringWithUTF8String:filename] : @"SONG.h";
    NSString *contents = text ? [NSString stringWithUTF8String:text] : @"";
    if (!requestedName.length) requestedName = @"SONG.h";
    if (!contents) contents = @"";
    if (![requestedName.pathExtension.lowercaseString isEqualToString:@"h"])
        requestedName = [requestedName stringByAppendingPathExtension:@"h"];
    NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent:requestedName.lastPathComponent];
    NSError *error = nil;
    if (![contents writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:&error]) {
        if (g_songSaved) g_songSaved(0);
        return;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController *presenter = TrackerTopViewController();
        if (!presenter) {
            if (g_songSaved) g_songSaved(0);
            return;
        }
        NSURL *fileURL = [NSURL fileURLWithPath:path];
        UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[fileURL] applicationActivities:nil];
        share.completionWithItemsHandler = ^(UIActivityType activityType, BOOL completed, NSArray *returnedItems, NSError *activityError) {
            (void)activityType; (void)returnedItems;
            if (g_songSaved) g_songSaved(completed && activityError == nil ? 1 : 0);
        };
        UIPopoverPresentationController *popover = share.popoverPresentationController;
        if (popover) {
            popover.sourceView = presenter.view;
            popover.sourceRect = CGRectMake(CGRectGetMidX(presenter.view.bounds), CGRectGetMidY(presenter.view.bounds), 1, 1);
            popover.permittedArrowDirections = 0;
        }
        [presenter presentViewController:share animated:YES completion:nil];
    });
}
