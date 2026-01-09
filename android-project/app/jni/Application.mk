
# Uncomment this if you're using STL in your project
# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support
# APP_STL := c++_shared

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

# APP_STL := c++_static
APP_STL := c++_shared
APP_CPPFLAGS := -std=c++17
APP_ABI := arm64-v8a armeabi-v7a x86 x86_64

APP_PLATFORM := android-21