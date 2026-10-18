#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>

extern "C" void PlatformAds_ShowOnLoss(void)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return;
    jclass cls = env->GetObjectClass(activity);
    jmethodID show = cls ? env->GetMethodID(cls, "showLossInterstitial", "()V") : nullptr;
    if (show) env->CallVoidMethod(activity, show);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (cls) env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
}
