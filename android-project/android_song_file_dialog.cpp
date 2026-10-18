#include <jni.h>
#include <SDL.h>
#include <SDL_system.h>

#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {
enum class ResultKind { Loaded, Closed, Saved };
struct SongFileResult {
    ResultKind kind;
    std::string filename;
    std::string text;
    bool success = false;
};
std::mutex resultMutex;
std::deque<SongFileResult> results;

void QueueResult(SongFileResult result)
{
    std::lock_guard<std::mutex> lock(resultMutex);
    results.push_back(std::move(result));
}

bool GetActivityMethod(JNIEnv *env, jobject &activity, jclass &activityClass,
                       const char *name, const char *signature, jmethodID &method)
{
    activity = static_cast<jobject>(SDL_AndroidGetActivity());
    if (!activity)
        return false;
    activityClass = env->GetObjectClass(activity);
    method = activityClass ? env->GetMethodID(activityClass, name, signature) : nullptr;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        method = nullptr;
    }
    if (!method) {
        if (activityClass) env->DeleteLocalRef(activityClass);
        env->DeleteLocalRef(activity);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Android tracker picker method missing: %s", name);
        return false;
    }
    return true;
}
}

extern "C" void Tracker_AndroidOpenSongFile(void)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = nullptr;
    jclass cls = nullptr;
    jmethodID method = nullptr;
    if (!env || !GetActivityMethod(env, activity, cls, "openTrackerSong", "()V", method)) {
        QueueResult({ResultKind::Closed});
        return;
    }
    env->CallVoidMethod(activity, method);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        QueueResult({ResultKind::Closed});
    }
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
}

extern "C" void Tracker_AndroidSaveSongFile(const char *filename, const char *text)
{
    auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject activity = nullptr;
    jclass cls = nullptr;
    jmethodID method = nullptr;
    if (!env || !GetActivityMethod(env, activity, cls, "saveTrackerSong", "(Ljava/lang/String;[B)V", method)) {
        QueueResult({ResultKind::Saved, {}, {}, false});
        return;
    }
    const std::string content = text ? text : "";
    jstring name = env->NewStringUTF(filename ? filename : "SONG.h");
    jbyteArray bytes = env->NewByteArray(static_cast<jsize>(content.size()));
    if (name && bytes) {
        env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(content.size()),
                                reinterpret_cast<const jbyte *>(content.data()));
        env->CallVoidMethod(activity, method, name, bytes);
    }
    if (!name || !bytes || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        QueueResult({ResultKind::Saved, {}, {}, false});
    }
    if (bytes) env->DeleteLocalRef(bytes);
    if (name) env->DeleteLocalRef(name);
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeSongLoaded(JNIEnv *env, jclass, jstring filename, jbyteArray contents)
{
    SongFileResult result{ResultKind::Loaded};
    if (filename && contents) {
        const char *name = env->GetStringUTFChars(filename, nullptr);
        if (name) {
            result.filename = name;
            env->ReleaseStringUTFChars(filename, name);
            const jsize size = env->GetArrayLength(contents);
            result.text.resize(static_cast<size_t>(size));
            if (size > 0)
                env->GetByteArrayRegion(contents, 0, size, reinterpret_cast<jbyte *>(&result.text[0]));
            result.success = !env->ExceptionCheck();
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }
    QueueResult(std::move(result));
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeSongPickerClosed(JNIEnv *, jclass)
{
    QueueResult({ResultKind::Closed});
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_TrackerActivity_nativeSongSaved(JNIEnv *, jclass, jboolean success)
{
    QueueResult({ResultKind::Saved, {}, {}, success == JNI_TRUE});
}

extern "C" void Tracker_AndroidPollSongFileResults(
    void (*loaded)(const char *, const char *),
    void (*closed)(void),
    void (*saved)(int))
{
    std::deque<SongFileResult> pending;
    {
        std::lock_guard<std::mutex> lock(resultMutex);
        pending.swap(results);
    }
    for (const SongFileResult &result : pending) {
        switch (result.kind) {
        case ResultKind::Loaded:
            if (loaded) loaded(result.success ? result.filename.c_str() : nullptr,
                               result.success ? result.text.c_str() : nullptr);
            break;
        case ResultKind::Closed:
            if (closed) closed();
            break;
        case ResultKind::Saved:
            if (saved) saved(result.success ? 1 : 0);
            break;
        }
    }
}
