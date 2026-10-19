#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>

extern "C" void Game_OnInterstitialShown(void);
extern "C" void Game_OnInterstitialFinished(void);
extern "C" void Game_OnMoneyRewardEarned(void);
extern "C" void Game_OnMoneyRewardFinished(void);

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

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeMoneyRewardEarned(JNIEnv *, jclass)
{
    Game_OnMoneyRewardEarned();
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeMoneyRewardFinished(JNIEnv *, jclass)
{
    Game_OnMoneyRewardFinished();
}

extern "C" bool PlatformAds_RewardedReady(void)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) return false;
    jclass cls = env->GetObjectClass(activity);
    jmethodID ready = cls ? env->GetMethodID(cls, "isRewardedMoneyAdReady", "()Z") : nullptr;
    const bool result = ready && env->CallBooleanMethod(activity, ready);
    if (env->ExceptionCheck()) { env->ExceptionClear(); }
    if (cls) env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return result;
}

extern "C" void PlatformAds_ShowMoneyReward(void)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!env || !activity) { Game_OnMoneyRewardFinished(); return; }
    jclass cls = env->GetObjectClass(activity);
    jmethodID show = cls ? env->GetMethodID(cls, "showRewardedMoneyAd", "()V") : nullptr;
    if (show) env->CallVoidMethod(activity, show);
    else Game_OnMoneyRewardFinished();
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        Game_OnMoneyRewardFinished();
    }
    if (cls) env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
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
