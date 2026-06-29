#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(void, js_storage_set, (const char *key, const char *val), {
    let k = UTF8ToString(key);
    let v = UTF8ToString(val);

    let data = localStorage.getItem("YourApp.settings");
    let obj = data ? JSON.parse(data) : {};
    obj[k] = v;
    localStorage.setItem("YourApp.settings", JSON.stringify(obj));
});

EM_JS(int, js_storage_get, (const char *key, char *out, int maxLen), {
    let k = UTF8ToString(key);
    let data = localStorage.getItem("YourApp.settings");
    if (!data)
        return 0;

    let obj = JSON.parse(data);
    if (!(k in obj))
        return 0;

    let v = obj[k];
    let len = lengthBytesUTF8(v);
    if (len >= maxLen)
        len = maxLen - 1;

    stringToUTF8(v, out, maxLen);
    return len;
});
#endif
#ifdef __EMSCRIPTEN__
void js_storage_set(const char *key, const char *val);
int js_storage_get(const char *key, char *out, int maxLen);
#endif

struct Storage
{
    enum KEYS_ENUM
    {
        USERNAME,
        TOKEN,
        LAST_LEVEL,
        SCHOOL_DONE,
        GREETINGS_SEEN,
        LANGUAGE,
        BANK,
        GAMEPLAY_TIME,
        UNLOCKED_BALLS,
        UNLOCKED_HOUSES,
        UNLOCKED_BOTS,
        EQUIPPED_BALL,
        CAMPAIGN_COMPLETED,
        CAMPAIGN_CLEAR_TIME,
        CAMPAIGN_LEVEL_ATTEMPTS,
        KEY_COUNT
    };

    static constexpr const char *keyNames[KEY_COUNT] = {
        "username",
        "token",
        "last_level",
        "school_done",
        "greetings_seen",
        "language",
        "bank",
        "gameplay_time",
        "unlocked_balls",
        "unlocked_houses",
        "unlocked_bots",
        "equipped_ball",
        "campaign_completed",
        "campaign_clear_time",
        "campaign_level_attempts"
    };

    static constexpr const char *defaultValues[KEY_COUNT] = {
        "guest", // USERNAME
        "",      // TOKEN
        "1",     // LAST_LEVEL
        "0",     // SCHOOL_DONE
        "0",     // GREETINGS_SEEN
        "en_us", // LANGUAGE
        "20",    // BANK
        "0",     // GAMEPLAY_TIME
        "0",     // UNLOCKED_BALLS
        "0",     // UNLOCKED_HOUSES
        "0",     // UNLOCKED_BOTS
        "0",     // EQUIPPED_BALL
        "0",     // CAMPAIGN_COMPLETED
        "0",     // CAMPAIGN_CLEAR_TIME
        "0,0,0,0,0,0,0,0,0,0,0,0,0" // CAMPAIGN_LEVEL_ATTEMPTS
    };
    char filePath[512];

    /**
     * Initialize storage
     */
    void storageInit(const char *company, const char *app)
    {
#ifdef __EMSCRIPTEN__
        // ensure defaults exist
        for (int i = 0; i < KEY_COUNT; ++i)
        {
            char tmp[256];
            if (getChar((KEYS_ENUM)i, tmp, sizeof(tmp)) == 0)
            {
                js_storage_set(keyNames[i], defaultValues[i]);
            }
        }
#else
        char *base = SDL_GetPrefPath(company, app);
        if (!base)
        {
            filePath[0] = 0;
            return;
        }
        snprintf(filePath, sizeof(filePath), "%ssettings.ini", base);
        SDL_free(base);
        fprintf(stdout, "Usinf settings file: %s\n", filePath);

        FILE *f = fopen(filePath, "r");
        if (!f)
        {
            f = fopen(filePath, "w");
            if (f)
            {
                for (int i = 0; i < KEY_COUNT; ++i)
                    fprintf(f, "%s=%s\n", keyNames[i], defaultValues[i]);
                fclose(f);
            }
        }
#endif
    }

    /**
     * Set entry
     */
    size_t setChar(KEYS_ENUM key, const char *val, size_t length)
    {
#ifdef __EMSCRIPTEN__
        char tmp[512];
        memcpy(tmp, val, length);
        tmp[length] = 0;
        js_storage_set(keyNames[key], tmp);
        return length;
#else
        char buffer[4096] = {0};

        FILE *f = fopen(filePath, "r");
        if (f)
        {
            fread(buffer, 1, sizeof(buffer) - 1, f);
            fclose(f);
        }

        char *out = buffer;
        char *line = strtok(buffer, "\n");

        char newFile[4096] = {0};
        size_t written = 0;
        bool replaced = false;

        while (line)
        {
            if (strncmp(line, keyNames[key], strlen(keyNames[key])) == 0 &&
                line[strlen(keyNames[key])] == '=')
            {
                written += snprintf(
                    newFile + written,
                    sizeof(newFile) - written,
                    "%s=%.*s\n",
                    keyNames[key],
                    (int)length,
                    val
                );
                replaced = true;
            }
            else
            {
                written += snprintf(newFile + written, sizeof(newFile) - written, "%s\n", line);
            }
            line = strtok(nullptr, "\n");
        }

        if (!replaced)
        {
            written += snprintf(
                newFile + written,
                sizeof(newFile) - written,
                "%s=%.*s\n",
                keyNames[key],
                (int)length,
                val
            );
        }

        f = fopen(filePath, "w");
        if (!f)
            return 0;

        fwrite(newFile, 1, written, f);
        fclose(f);
        return length;
#endif
    }

    /**
     * Get entry
     */
    size_t getChar(KEYS_ENUM key, char *outVal, size_t maxLen)
    {
#ifdef __EMSCRIPTEN__
        return js_storage_get(keyNames[key], outVal, (int)maxLen);
#else
        FILE *f = fopen(filePath, "r");
        if (!f)
            return 0;

        char line[512];

        while (fgets(line, sizeof(line), f))
        {
            if (strncmp(line, keyNames[key], strlen(keyNames[key])) == 0 &&
                line[strlen(keyNames[key])] == '=')
            {
                const char *val = line + strlen(keyNames[key]) + 1;
                size_t len = strlen(val);

                if (len && val[len - 1] == '\n')
                    len--;

                if (len > maxLen)
                    len = maxLen;

                memcpy(outVal, val, len);
                outVal[len] = 0;

                fclose(f);
                return len;
            }
        }

        fclose(f);
        return 0;
#endif
    }

    size_t reset()
    {
#ifdef __EMSCRIPTEN__
        EM_ASM({ localStorage.removeItem("YourApp.settings"); });

        // reapply defaults
        for (int i = 0; i < KEY_COUNT; ++i)
            js_storage_set(keyNames[i], defaultValues[i]);

        return KEY_COUNT;
#else
        FILE *f = fopen(filePath, "w");
        if (!f)
            return 0;

        for (int i = 0; i < KEY_COUNT; ++i)
            fprintf(f, "%s=%s\n", keyNames[i], defaultValues[i]);

        fclose(f);
        return KEY_COUNT;
#endif
    }
};
