LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ./3rdparty/SDL
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

GLM_PATH := ./3rdparty/glm
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(GLM_PATH)

# GASLIGHT_PATH :=
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../3rdparty/glm

# Add your application source files here...
LOCAL_SRC_FILES := android_gaslight.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_CPPFLAGS := -std=c++17
LOCAL_CFLAGS    := -std=c++17 -Wall -Wextra
LOCAL_LDLIBS := -lGLESv3 -lOpenSLES -llog -landroid
# LOCAL_LDLIBS += -lc++  # may be needed on some NDKs

include $(BUILD_SHARED_LIBRARY)

$(call import-module,3rdparty/SDL)


