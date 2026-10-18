#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>

extern "C" void Game_OnInterstitialShown(void);
extern "C" void Game_OnInterstitialFinished(void);

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeInterstitialShown(JNIEnv *, jclass)
{
    Game_OnInterstitialShown();
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeInterstitialFinished(JNIEnv *, jclass)
{
    Game_OnInterstitialFinished();
}

extern "C" void PlatformAds_ShowInterstitial(void)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) {
        Game_OnInterstitialFinished();
        return;
    }
    jclass cls = env->GetObjectClass(activity);
    jmethodID show = cls ? env->GetMethodID(cls, "showGameInterstitial", "()V") : nullptr;
    if (show) env->CallVoidMethod(activity, show);
    else Game_OnInterstitialFinished();
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        Game_OnInterstitialFinished();
    }
    if (cls) env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
}
