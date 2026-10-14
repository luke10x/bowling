#import <Cocoa/Cocoa.h>

#include <string>

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

static NSString *TrackerNSString(const char *value)
{
    if (!value)
        return @"";
    return [NSString stringWithUTF8String:value] ?: @"";
}

extern "C" void Tracker_NativeOpenSongFile(void)
{
    @autoreleasepool {
        [NSApp activateIgnoringOtherApps:YES];
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.canChooseFiles = YES;
        panel.canChooseDirectories = NO;
        panel.allowsMultipleSelection = NO;
        panel.allowedFileTypes = @[@"h", @"txt"];
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK || panel.URL == nil) {
            if (g_songClosed) g_songClosed();
            return;
        }
        NSError *error = nil;
        NSString *contents = [NSString stringWithContentsOfURL:panel.URL encoding:NSUTF8StringEncoding error:&error];
        if (!contents) {
            if (g_songLoaded) g_songLoaded(panel.URL.lastPathComponent.UTF8String, nullptr);
            return;
        }
        if (g_songLoaded) g_songLoaded(panel.URL.lastPathComponent.UTF8String, contents.UTF8String);
    }
}

extern "C" void Tracker_NativeSaveSongFile(const char *filename, const char *text)
{
    NSString *name = TrackerNSString(filename);
    NSString *contents = TrackerNSString(text);
    @autoreleasepool {
        [NSApp activateIgnoringOtherApps:YES];
        NSSavePanel *panel = [NSSavePanel savePanel];
        panel.canCreateDirectories = YES;
        panel.allowedFileTypes = @[@"h"];
        panel.nameFieldStringValue = name.length ? name : @"SONG.h";
        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK || panel.URL == nil) {
            if (g_songSaved) g_songSaved(0);
            return;
        }
        NSError *error = nil;
        BOOL saved = [contents writeToURL:panel.URL atomically:YES encoding:NSUTF8StringEncoding error:&error];
        if (g_songSaved) g_songSaved(saved ? 1 : 0);
    }
}
