#!/bin/bash
today=$(date '+%Y-%m-%d, %a')
echo -e "\n## $today" >> "$0"
exit 0

## 2025-08-04, Mon

This file will be journaling Android port development

Seme initial resources:

- https://stackoverflow.com/questions/30852746/try-to-set-sdl-with-opengl-context-on-android-get-error-message-failed-loading
- https://lisyarus.github.io/blog/posts/porting-for-android.html

As indicated in the first link:
i am following this yt:[]()


## 2025-08-24, Sun

Links.
1) SDL 2.0. http://www.libsdl.org/tmp/download-2....
2) Android Development Kit. http://developer.android.com/sdk/inde...
3) Android Native Development Kit (NDK) http://developer.android.com/tools/sd...

1. Copied android project from external/SDL/android-project
2. Open it in Android Studio
3. NDK Tools install in Android Studio:
    Menu: Android Studio -> Settings -> Languages & Frameworks -> Android SDK
    Tab: SDK Tools
    [x] NDK (Side by side)
    [x] CMake
   Will be 1GB download
4. Edit /Users/lape/workspace/big-jump-nrg/android-project/app/jni/Application.mk
   To add:
        APP_STL := c++_static
        APP_PLATFORM := android-21
        **probably not required**

5. edit android-project/app/jni/src/Android.mk 
    I guess now it is edited and it does not need to be done again
6. Run the NDK build
    export NDK_MODULE_PATH=$(pwd)/external
    export NDK_PROJECT_PATH=$(pwd)/android-project/app
    /Users/lape/Library/Android/sdk/ndk/29.0.13846066/ndk-build
    open -a "Android Studio"
7. Add file jni/src/main.cpp
   in my case it is android-project/app/jni/src/android_main.cpp

## 2025-09-12, Fri

Fix step 6:

    # Make sure you are NOT in Nix environment
    export NDK_MODULE_PATH=$(pwd)
    export NDK_PROJECT_PATH=$(pwd)/android-project/app
    open -a "Android Studio"

    
## 2026-01-08, Thu

I just copied this Android Project from Galsight Runner.
Only to find that it does not work any more.
I had to migrate to CMake.txt, looks like it is better supported.
I made it work, I will commit it now, But real clanup oof NDK will follow after.
And finally I don't need to run Android Studio from configured environment